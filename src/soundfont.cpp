#include <algorithm>

#include <chrono>
#include <iostream>
#include "prestosynth/soundfont.h"
#include "prestosynth/envelope.h"
#include "prestosynth/lfo.h"
#include "prestosynth/filter.h"
#include "prestosynth/util/math_util.h"

namespace psynth {

// Function to convert SF2 timecents to seconds (using std::pow)
inline float timecents_to_s_corrected(int16_t tc) {
    // SF2 spec: -32768 indicates 0 time
    if (tc <= -32768) return 0.0f;
    // Clamp to reasonable min/max to avoid extreme values from pow
    // Min: -12000 (approx 0.001s), Max: 8000 (approx 17s for decay/release), 5000 (approx 2.8s for hold/delay)
    // We can apply more specific clamps in Envelope::configure later.
    float clamped_tc = std::clamp(static_cast<float>(tc), -12000.f, 8000.f);
    return std::pow(2.f, clamped_tc / 1200.f);
}

// Function to convert SF2 absolute cents to Hz
inline float abscents_to_hz_corrected(int16_t ac) {
    // 13500 cents is often treated as the max (~20kHz)
    // Min practical frequency is ~8.176 Hz (MIDI note 0)
    float clamped_ac = std::clamp(static_cast<float>(ac), 0.f, 13500.f);
    return 8.176f * std::pow(2.f, clamped_ac / 1200.f);
}

// Function to convert SF2 centibels (cB) to linear gain
// Note: SF2 InitialFilterQ is in cB (tenths of dB)
inline float cb_to_linear_q_gain(int16_t cb) {
    // Filter Q (resonance) is often interpreted as dB peak height.
    // A value of 0 cB (0 dB) should ideally result in a non-resonant filter (Q=1/sqrt(2)).
    // Higher values increase resonance.
    // We pass the raw dB value (cb / 10.0f) to the LowPassFilter constructor,
    // which will handle the conversion to internal Q factor.
    return static_cast<float>(cb) / 10.f; // Return dB value
}

// Function to convert SF2 relative cents to tuning ratio
inline float cents_to_ratio(int cents) {
    return std::pow(2.f, static_cast<float>(cents) / 1200.f);
}

// Function to convert SF2 ModLfoToVolume (cB) to linear gain adjustment
inline float mod_lfo_to_volume_gain(int16_t cb) {
    // This represents the maximum gain deviation in dB due to LFO.
    // Convert cB to dB, then to linear gain ratio.
    float db = static_cast<float>(cb) / 10.f;
    return std::pow(10.f, db / 20.f); // This is the gain multiplier at LFO peak (1.0)
                                      // The actual gain adjustment will be `lfo_value * (gain_multiplier - 1.0)` ?
                                      // Or is it additive in dB space? SF2 spec is ambiguous.
                                      // TinySoundFont/FluidSynth treat it as gain in dB to add: gain_db = base_gain_db + lfo_value * modlfo_db
                                      // Let's return the dB value for now.
    return db;
}

inline void PrestoSoundFont::handle_smpl(sf_internal::GeneratorPack presetInfo, sf_internal::GeneratorPack instInfo, const PresetHead pHead, uint16_t smplIdx) {
    using namespace sf_internal;
    constexpr std::array<uint16_t, 38> ADDITIVE_GEN_IDXS {5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 48, 51, 52};

    const auto &smplInfo = sf.shdr(smplIdx);

    for(int i = 0; i < ADDITIVE_GEN_IDXS.size(); ++i) {
        if (presetInfo[ADDITIVE_GEN_IDXS[i]].sAmount != defaultGenPack[ADDITIVE_GEN_IDXS[i]].sAmount) {
             instInfo[ADDITIVE_GEN_IDXS[i]].sAmount += presetInfo[ADDITIVE_GEN_IDXS[i]].sAmount;
        }
    }

    uint8_t keyLow = (instInfo[Keynum].sAmount != -1) ? static_cast<uint8_t>(instInfo[Keynum].sAmount) : 
                      (presetInfo[KeyRange].lowByte != defaultGenPack[KeyRange].lowByte || instInfo[KeyRange].lowByte != defaultGenPack[KeyRange].lowByte) ? std::max(presetInfo[KeyRange].lowByte, instInfo[KeyRange].lowByte) : defaultGenPack[KeyRange].lowByte;
    uint8_t keyHigh = (instInfo[Keynum].sAmount != -1) ? static_cast<uint8_t>(instInfo[Keynum].sAmount) : 
                       (presetInfo[KeyRange].highByte != defaultGenPack[KeyRange].highByte || instInfo[KeyRange].highByte != defaultGenPack[KeyRange].highByte) ? std::min(presetInfo[KeyRange].highByte, instInfo[KeyRange].highByte) : defaultGenPack[KeyRange].highByte;
    uint8_t velLow = (instInfo[Velocity].sAmount != -1) ? static_cast<uint8_t>(instInfo[Velocity].sAmount) : 
                      (presetInfo[VelRange].lowByte != defaultGenPack[VelRange].lowByte || instInfo[VelRange].lowByte != defaultGenPack[VelRange].lowByte) ? std::max(presetInfo[VelRange].lowByte, instInfo[VelRange].lowByte) : defaultGenPack[VelRange].lowByte;
    uint8_t velHigh = (instInfo[Velocity].sAmount != -1) ? static_cast<uint8_t>(instInfo[Velocity].sAmount) : 
                       (presetInfo[VelRange].highByte != defaultGenPack[VelRange].highByte || instInfo[VelRange].highByte != defaultGenPack[VelRange].highByte) ? std::min(presetInfo[VelRange].highByte, instInfo[VelRange].highByte) : defaultGenPack[VelRange].highByte;

    int32_t startAddrOfs = instInfo[StartAddrsOffset].sAmount + instInfo[StartAddrsCoarseOffset].sAmount * 32768;
    int32_t endAddrOfs = instInfo[EndAddrsOffset].sAmount + instInfo[EndAddrsCoarseOffset].sAmount * 32768;
    int32_t startLoopAddrOfs = instInfo[StartloopAddrsOffset].sAmount + instInfo[StartloopAddrsCoarseOffset].sAmount * 32768;
    int32_t endLoopAddrOfs = instInfo[EndloopAddrsOffset].sAmount + instInfo[EndloopAddrsCoarseOffset].sAmount * 32768;

    SampleAttribute attr = {
        smplInfo.originalPitch,
        static_cast<LoopMode>(instInfo[SampleModes].sAmount),
        (instInfo[ScaleTuning].uAmount == 100),

        smplInfo.sampleRate,
        static_cast<uint32_t>(std::max(0, static_cast<int>(smplInfo.startOffset) + startAddrOfs)),
        static_cast<uint32_t>(std::max(0, static_cast<int>(smplInfo.endOffset) + endAddrOfs)),
        static_cast<uint32_t>(std::max(0, static_cast<int>(smplInfo.startLoop) + startLoopAddrOfs)),
        static_cast<uint32_t>(std::max(0, static_cast<int>(smplInfo.endLoop) + endLoopAddrOfs)),

        std::clamp(static_cast<float>(instInfo[Pan].sAmount) / 1000.f, -0.5f, 0.5f),
        std::pow(10.f, -static_cast<float>(instInfo[InitialAttenuation].sAmount) / 200.f),

        static_cast<float>(instInfo[FineTune].sAmount + smplInfo.pitchCorrection) + static_cast<float>(instInfo[CoarseTune].sAmount) * 100.f,

        timecents_to_s_corrected(instInfo[DelayVolEnv].sAmount),
        timecents_to_s_corrected(instInfo[AttackVolEnv].sAmount),
        timecents_to_s_corrected(instInfo[HoldVolEnv].sAmount),
        timecents_to_s_corrected(instInfo[DecayVolEnv].sAmount),
        std::max(0.f, 1.f - static_cast<float>(instInfo[SustainVolEnv].sAmount) / 1000.f),
        timecents_to_s_corrected(instInfo[ReleaseVolEnv].sAmount),

        timecents_to_s_corrected(instInfo[DelayModEnv].sAmount),
        timecents_to_s_corrected(instInfo[AttackModEnv].sAmount),
        timecents_to_s_corrected(instInfo[HoldModEnv].sAmount),
        timecents_to_s_corrected(instInfo[DecayModEnv].sAmount),
        std::max(0.f, 1.f - static_cast<float>(instInfo[SustainModEnv].sAmount) / 1000.f),
        timecents_to_s_corrected(instInfo[ReleaseModEnv].sAmount),

        timecents_to_s_corrected(instInfo[DelayModLFO].sAmount),
        abscents_to_hz_corrected(instInfo[FreqModLFO].sAmount),

        timecents_to_s_corrected(instInfo[DelayVibLFO].sAmount),
        abscents_to_hz_corrected(instInfo[FreqVibLFO].sAmount),

        abscents_to_hz_corrected(instInfo[InitialFilterFc].sAmount),
        cb_to_linear_q_gain(instInfo[InitialFilterQ].sAmount),
        static_cast<float>(instInfo[ModLfoToFilterFc].sAmount),
        static_cast<float>(instInfo[ModEnvToFilterFc].sAmount),

        mod_lfo_to_volume_gain(instInfo[ModLfoToVolume].sAmount),
        static_cast<float>(instInfo[VibLfoToPitch].sAmount)
    };

    if (attr.endLoop <= attr.startLoop) {
        attr.loopMode = LoopMode::NoLoop;
    }

    presetIdx[pHead].emplace_back(SampleInfo{
        { keyLow, velLow, keyHigh, velHigh },
        attr
    });
}

inline void PrestoSoundFont::handle_inst(sf_internal::GeneratorPack presetInfo, const PresetHead pHead, uint16_t instIdx) {
    sf_internal::GeneratorPack iGlobalInfo = sf_internal::defaultGenPack;

    const auto &thisInst = sf.inst(instIdx);
    const auto &nextInst = sf.inst(instIdx + 1);
    for(uint32_t iBagIdx = thisInst.bagIdx; iBagIdx < nextInst.bagIdx; iBagIdx++) {
        sf_internal::GeneratorPack iZoneInfo = iGlobalInfo;
        bool iGlobal = true;

        for(uint16_t genIdx = sf.ibag(iBagIdx).genIdx; genIdx < sf.ibag(iBagIdx + 1).genIdx; genIdx++) {
            const auto &iGen = sf.igen(genIdx);
            iZoneInfo[iGen.genOper].sAmount = iGen.amount.sAmount;

            if(iGen.genOper == sf_internal::GeneratorType::SampleID) {
                iGlobal = false;
                handle_smpl(presetInfo, iZoneInfo, pHead, iGen.amount.uAmount);

                break;
            }
        }

        if(iBagIdx == thisInst.bagIdx &&
            iGlobal) {
            iGlobalInfo = iZoneInfo;
        }
    }
};

inline void PrestoSoundFont::handle_phdr() {
    for(auto pIdx = 0; pIdx < sf.phdr_num() - 1; ++pIdx) {
        const auto &thisPreset = sf.phdr(pIdx);
        const auto &nextPreset = sf.phdr(pIdx + 1);
        PresetHead pHead = static_cast<uint16_t>((thisPreset.bankNum << 8) | thisPreset.presetNum);
        presetIdx.insert({{ pHead }, SampleInfos()});

        sf_internal::GeneratorPack pGlobalInfo = sf_internal::defaultGenPack;
        bool pGlobal = true;
        for(uint32_t pBagIdx = thisPreset.bagIdx; pBagIdx < nextPreset.bagIdx; ++pBagIdx) {
            sf_internal::GeneratorPack pZoneInfo = pGlobalInfo;

            for(uint16_t genIdx = sf.pbag(pBagIdx).genIdx; genIdx < sf.pbag(pBagIdx + 1).genIdx; genIdx++) {
                const auto &pGen = sf.pgen(genIdx);
                pZoneInfo[pGen.genOper].sAmount = pGen.amount.sAmount;

                if(pGen.genOper == sf_internal::GeneratorType::Instrument) {
                    pGlobal = false;
                    handle_inst(pZoneInfo, pHead, pGen.amount.uAmount);

                    break;
                }
            }

            if(pBagIdx == thisPreset.bagIdx &&
                pGlobal) {
                pGlobalInfo = pZoneInfo;
            }
        }
    };
};

PrestoSoundFont::PrestoSoundFont(const std::string &filepath, uint32_t sampleRate, uint8_t quality):
    sf(sf_internal::SoundFont(filepath)), sampleRate(sampleRate), quality(quality) {
    handle_phdr();
};

const SampleInfoPack PrestoSoundFont::get_sample_info(uint8_t preset, uint8_t bank, uint8_t pitch, uint8_t velocity) const {
    PresetHead pHead = static_cast<uint16_t>((bank << 8) | preset);
    const auto resPreset = presetIdx.find(pHead);
    SampleInfoPack res;

    if(resPreset == presetIdx.end())
        return res;

    for(const auto &sampleInfo : resPreset->second) {
        if(sampleInfo.head.pitchLow <= pitch &&
            sampleInfo.head.pitchHigh >= pitch &&
            sampleInfo.head.velLow <= velocity &&
            sampleInfo.head.velHigh >= velocity)
            res.emplace_back(&sampleInfo.attr);
    }

    return res;
};

const Sample PrestoSoundFont::get_raw_sample(const SampleAttribute &sampleAttr, uint8_t pitch) {
    std::lock_guard<std::mutex> lk(cacheMtx);

    uint32_t cacheKey = pitch | (sampleAttr.startOffset << 8);
    auto cachedSample = sampleCache.find(cacheKey);
    if(cachedSample != sampleCache.end()) {
        if (cachedSample->second.attr.pitch == pitch) {
             return cachedSample->second;
        }
    }

    AudioData sample = sf.sample(
        sampleAttr.startOffset,
        sampleAttr.endOffset - sampleAttr.startOffset,
        sampleAttr.sampleRate,
        sampleRate,
        quality);

    float baseShift = std::pow(2.f, (static_cast<float>(pitch) - sampleAttr.pitch) / 12.f);
    float tuneShift = cents_to_ratio(static_cast<int>(sampleAttr.tune));
    float shiftRatio = baseShift * tuneShift;

    if (std::abs(shiftRatio - 1.0f) > 1e-6) {
         sample = resample_mono(sample, shiftRatio, quality);
    }

    sample *= sampleAttr.attenuation;

    Sample rawSample {
        sampleAttr,
        sample
    };

    float effectiveResampleRatio = shiftRatio * (static_cast<float>(sampleRate) / static_cast<float>(sampleAttr.sampleRate));

    if (sf.version() == 2) {
         rawSample.attr.startLoop -= sampleAttr.startOffset;
         rawSample.attr.endLoop -= sampleAttr.startOffset;
    }

    rawSample.attr.startLoop = static_cast<uint32_t>(std::round(static_cast<float>(rawSample.attr.startLoop) * effectiveResampleRatio));
    rawSample.attr.endLoop = static_cast<uint32_t>(std::round(static_cast<float>(rawSample.attr.endLoop) * effectiveResampleRatio));

    rawSample.attr.pitch = pitch;
    rawSample.attr.sampleRate = sampleRate;

    if (rawSample.attr.endLoop >= rawSample.audio.cols()) {
        rawSample.attr.endLoop = rawSample.audio.cols() > 0 ? rawSample.audio.cols() - 1 : 0; // Corrected potential off-by-one
    }
     if (rawSample.attr.startLoop >= rawSample.attr.endLoop) {
        rawSample.attr.loopMode = LoopMode::NoLoop;
        rawSample.attr.startLoop = 0;
        rawSample.attr.endLoop = 0;
    }

    sampleCache[cacheKey] = rawSample;

    return rawSample;
};

// Implementation of time-variant resampler
static AudioData resample_time_variant(
    const AudioData& original_audio,
    float original_sample_rate,
    float target_sample_rate,
    float base_pitch_ratio, // Combined ratio from note diff, tuning, and SR conversion
    const AudioData& pitch_offset_cents_curve,
    uint32_t loop_start,
    uint32_t loop_end,
    sf_internal::LoopMode loop_mode,
    uint32_t target_length,
    uint8_t quality
) {
    AudioData resampled_audio = AudioData::Zero(1, target_length);
    if (original_audio.cols() == 0 || target_length == 0) {
        return resampled_audio; // Return silence if source or target is empty
    }

    double current_read_pos = 0.0; // Current read position in original_audio (double for precision)
    const double pitch_ratio_sr_factor = static_cast<double>(original_sample_rate) / target_sample_rate;
    const double loop_length = (loop_end > loop_start) ? static_cast<double>(loop_end - loop_start) : 0.0;
    const long source_length = original_audio.cols();
    bool is_looping_enabled = (loop_mode != sf_internal::LoopMode::NoLoop &&
                               loop_mode != sf_internal::LoopMode::Unused &&
                               loop_length > 0.0);

    // Ensure loop points are valid within source length
    loop_start = std::min(loop_start, static_cast<uint32_t>(source_length > 0 ? source_length - 1 : 0));
    loop_end = std::min(loop_end, static_cast<uint32_t>(source_length > 0 ? source_length - 1 : 0));
    if (loop_start >= loop_end) {
        is_looping_enabled = false;
    }

    const float* source_data = original_audio.data();
    float* target_data = resampled_audio.data();
    const float* cents_offset_data = pitch_offset_cents_curve.data();

    for (uint32_t i = 0; i < target_length; ++i) {
        // 1. Calculate instantaneous pitch ratio
        float cents_offset = (i < pitch_offset_cents_curve.cols()) ? cents_offset_data[i] : 0.f; // Get LFO offset for this sample
        double lfo_pitch_ratio = std::pow(2.0, static_cast<double>(cents_offset) / 1200.0);
        double instantaneous_ratio = base_pitch_ratio * lfo_pitch_ratio;
        double step = instantaneous_ratio * pitch_ratio_sr_factor;

        // 2. Clamp read position to avoid reading past the end before looping
        // If not looping, clamp strictly within bounds.
        // If looping, allow reading up to loop_end.
        double max_read_pos_before_wrap = static_cast<double>(source_length - 1);
        if (is_looping_enabled && loop_mode == sf_internal::LoopMode::Coutinuous) {
            // In continuous loop, the effective end is the loop_end
            max_read_pos_before_wrap = static_cast<double>(loop_end);
        }

        // Ensure read position doesn't exceed bounds needed for interpolation (need pos and pos+1)
        if (current_read_pos > max_read_pos_before_wrap - 1.001) { // Subtract 1 + epsilon for interpolation
            if (!is_looping_enabled) {
                // Reached end and not looping, fill rest with zero
                for (uint32_t j = i; j < target_length; ++j) target_data[j] = 0.f;
                break; // Exit outer loop
            }
             else if (loop_mode == sf_internal::LoopMode::Coutinuous) {
                 // Wrap around for continuous loop
                 current_read_pos -= loop_length; // Wrap back by loop length
                 // Re-check bounds after wrapping, prevent infinite loops if step is too small/negative or loop invalid
                 if (current_read_pos < loop_start || loop_length <= 0) {
                     current_read_pos = static_cast<double>(loop_start);
                 }
             } else { // LoopMode::ToEnd (or invalid state)
                 // Reached end, stop looping
                 is_looping_enabled = false;
                 // Clamp to end for the last sample read
                 current_read_pos = std::min(current_read_pos, static_cast<double>(source_length - 1.001));
             }
        }
        // Also check lower bound (can happen with negative pitch shift?)
        current_read_pos = std::max(0.0, current_read_pos);

        // 3. Interpolation (Linear for quality = 0)
        long read_pos_int = static_cast<long>(current_read_pos);
        double fraction = current_read_pos - read_pos_int;

        // Ensure we don't read past the actual buffer end, even with interpolation
        long next_pos_int = std::min(read_pos_int + 1, source_length - 1);
        read_pos_int = std::min(read_pos_int, source_length - 1);

        float sample1 = source_data[read_pos_int];
        float sample2 = source_data[next_pos_int];

        target_data[i] = sample1 + static_cast<float>(fraction * (sample2 - sample1));

        // 4. Advance read position
        current_read_pos += step;

        // 5. Handle Loop (Continuous mode check)
        // This check should happen *after* advancing, before the next iteration's read
        if (is_looping_enabled && loop_mode == sf_internal::LoopMode::Coutinuous) {
            while (current_read_pos >= loop_end && loop_length > 0.0) { // Use >= loop_end
                current_read_pos -= loop_length;
            }
        }
        // LoopMode::ToEnd doesn't require explicit wrap-around here, the bounds check handles it.
    }

    return resampled_audio;
}

AudioData PrestoSoundFont::build_sample(
    const SampleAttribute &attr, uint8_t pitch,
    uint8_t velocity, uint32_t /* durationFrames (unused in offline mode) */) {

    // 1. Get Base RAW Sample (no pitch shift yet, original SR)
    // Note: get_raw_sample now only fetches raw audio + applies attenuation.
    const Sample &rawSample = get_raw_sample(attr, pitch); // Pitch arg is now ignored by get_raw_sample
    if (rawSample.audio.cols() == 0) {
        return AudioData(); // Return empty if sample fetch failed
    }
    const AudioData& original_audio = rawSample.audio;
    const auto& original_attr = rawSample.attr; // Contains original SR, loop points etc.

    // 2. Determine Total Length Required (same as before)
    Envelope volEnv(static_cast<float>(sampleRate)); // Target sample rate
    volEnv.configure(
        attr.delayVol, attr.attackVol, attr.holdVol, attr.decayVol, attr.sustainVol, attr.releaseVol
    );
    uint32_t estimatedFrames = s_to_frames(attr.delayVol + attr.attackVol + attr.holdVol + attr.decayVol + attr.releaseVol + 1.0f, sampleRate);
    // Base length on original audio length adjusted for rough pitch shift, or envelope length
    float roughBaseShift = std::pow(2.f, (static_cast<float>(pitch) - original_attr.pitch) / 12.f);
    uint32_t estimatedAudioFrames = static_cast<uint32_t>(original_audio.cols() / (roughBaseShift * (static_cast<float>(original_attr.sampleRate) / sampleRate)));

    uint32_t totalLength = std::max(estimatedAudioFrames, estimatedFrames);
    totalLength = std::max(totalLength, s_to_frames(attr.delayVol + attr.attackVol + attr.holdVol + attr.decayVol + attr.releaseVol, sampleRate) + 100); // Ensure min length

    // 3. Prepare Modulators (Envelopes & LFOs)
    Envelope modEnv(static_cast<float>(sampleRate));
    modEnv.configure(attr.delayMod, attr.attackMod, attr.holdMod, attr.decayMod, attr.sustainMod, attr.releaseMod);
    AudioData modEnvCurve = modEnv.generateCurve(totalLength);

    LFO modLFO(attr.delayModLfo, attr.freqModLfo, static_cast<float>(sampleRate));
    AudioData modLFOCurve = modLFO(totalLength);

    LFO vibLFO(attr.delayVibLfo, attr.freqVibLfo, static_cast<float>(sampleRate));
    AudioData vibLFOCurve = vibLFO(totalLength);

    // Combine VibLFO curve with VibLfoToPitch generator value
    AudioData pitch_offset_cents_curve = vibLFOCurve * attr.vibLfoToPitch;

    // 4. Calculate Base Pitch Ratio for Resampler
    // Includes difference from target pitch, SF tuning, but NOT SR conversion (handled inside resampler)
    float baseShift = std::pow(2.f, (static_cast<float>(pitch) - original_attr.pitch) / 12.f);
    float tuneShift = cents_to_ratio(static_cast<int>(original_attr.tune)); // Use original_attr.tune
    float base_pitch_ratio = baseShift * tuneShift;

    // 5. Perform Time-Variant Resampling (Replaces old get_raw_sample pitch + loop)
    AudioData sample = resample_time_variant(
        original_audio,
        static_cast<float>(original_attr.sampleRate),
        static_cast<float>(sampleRate),
        base_pitch_ratio,
        pitch_offset_cents_curve,
        original_attr.startLoop, // Use original loop points from raw sample
        original_attr.endLoop,
        original_attr.loopMode,
        totalLength,
        quality
    );

    if (sample.cols() == 0) return sample; // Check if resampling failed
    totalLength = sample.cols(); // Update totalLength based on actual resampled result

    // Update curves if length changed (resampler might return slightly different length)
    if (modEnvCurve.cols() != totalLength) modEnvCurve = modEnv.generateCurve(totalLength);
    if (modLFOCurve.cols() != totalLength) modLFOCurve = modLFO(totalLength);
    // pitch_offset_cents_curve is implicitly handled if vibLFOCurve is regenerated

    // 6. Calculate Final Filter Frequency Curve (using updated totalLength)
    AudioData freqCurveHz(1, totalLength);
    for (long i = 0; i < totalLength; ++i) {
        float baseFc = attr.initFilterFc;
        // Modulations add in cents space
        float combinedFcCents = psynth::hz_to_cents(baseFc) +
                                (i < modEnvCurve.cols() ? modEnvCurve(i) : 0.f) * attr.modEnvToFilterFc +
                                (i < modLFOCurve.cols() ? modLFOCurve(i) : 0.f) * attr.modLfoToFilterFc;
        freqCurveHz(i) = psynth::cents_to_hz(std::max(0.f, combinedFcCents));
    }

    // 7. Process Low-Pass Filter
    if (attr.filterQ > 0.01f || attr.initFilterFc < 19900.f) {
        LowPassFilter filter(attr.filterQ, static_cast<float>(sampleRate));
        filter.process(sample, freqCurveHz);
    }

    // 8. Generate Volume Envelope Curve (using updated totalLength)
    AudioData volEnvCurve = volEnv.generateCurve(totalLength);

    // 9. Apply Volume Envelope and Mod LFO to Volume
    float modLfoVolDb = attr.modLfoToVolume; // This is dB value from handle_smpl now
    for (long i = 0; i < totalLength; ++i) {
        float volEnvGain = (i < volEnvCurve.cols()) ? volEnvCurve(i) : 0.f;
        float lfoValue = (i < modLFOCurve.cols()) ? modLFOCurve(i) : 0.f;
        float lfoVolDbAdjust = lfoValue * modLfoVolDb;
        float finalGain = volEnvGain * std::pow(10.f, lfoVolDbAdjust / 20.f);
        sample(i) *= std::max(0.f, finalGain);
    }

    // 10. Trim trailing zeros (same as before)
    long lastNonZero = totalLength - 1;
    while (lastNonZero >= 0 && std::abs(sample(lastNonZero)) < 1e-7) {
        lastNonZero--;
    }
    if (lastNonZero < totalLength - 1 && lastNonZero >= 0) {
        sample.conservativeResize(Eigen::NoChange, lastNonZero + 1);
    }
     else if (lastNonZero < 0) {
         return AudioData(1,1);
     }

    return sample;
};

AudioData PrestoSoundFont::build_note(
    uint8_t preset, uint8_t bank, uint8_t pitch, uint8_t velocity,
    uint32_t durationFrames, bool stereo) {
    const SampleInfoPack sampleInfos = get_sample_info(preset, bank, pitch, velocity);

    uint32_t maxFrames = 0;
    std::vector<AudioData> samples;
    samples.reserve(sampleInfos.size());
    for(auto sampleInfoAttrPtr : sampleInfos) {
        AudioData thisSample = build_sample(*sampleInfoAttrPtr, pitch, velocity, 0);

        if(thisSample.cols() > 0) {
             if(thisSample.cols() > maxFrames)
                 maxFrames = thisSample.cols();
             samples.emplace_back(std::move(thisSample));
        } else {
             samples.emplace_back();
        }
    }

    if (maxFrames == 0) {
        return AudioData::Zero(stereo ? 2 : 1, 1);
    }

    AudioData outputSample = AudioData::Zero(stereo ? 2 : 1, maxFrames);
    for(size_t i = 0; i < sampleInfos.size(); i++) {
        AudioData &sample = samples[i];
        if (sample.cols() == 0) continue;

        const auto& attr = *sampleInfos[i];
        float pan = attr.pan;

        float gainL = 0.5f - pan;
        float gainR = 0.5f + pan;

        if(stereo) {
            gainL = std::clamp(gainL, 0.f, 1.f);
            gainR = std::clamp(gainR, 0.f, 1.f);
            outputSample.row(0).leftCols(sample.cols()) += sample.row(0) * gainL * 2.f;
            outputSample.row(1).leftCols(sample.cols()) += sample.row(0) * gainR * 2.f;
        } else {
            outputSample.leftCols(sample.cols()) += sample;
        }
    }

    return outputSample;
};

}
