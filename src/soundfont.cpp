#include <algorithm>

#include <chrono>
#include <iostream>
#include "prestosynth/soundfont.h"
#include "prestosynth/envelope.h"
#include "prestosynth/filter.h"
#include "prestosynth/util/math_util.h"

namespace psynth {

inline void PrestoSoundFont::handle_smpl(sf_internal::GeneratorPack presetInfo, sf_internal::GeneratorPack instInfo, const PresetHead pHead, uint16_t smplIdx) {
    using namespace sf_internal;
    constexpr std::array<uint16_t, 38> ADDITIVE_GEN_IDXS {5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 48, 51, 52};

    const auto &smplInfo = sf.shdr(smplIdx);

    for(int i = 0; i < ADDITIVE_GEN_IDXS.size(); ++i) {
        // if(presetInfo[ADDITIVE_GEN_IDXS[i]].sAmount != defaultGenPack[ADDITIVE_GEN_IDXS[i]].sAmount)
            instInfo[ADDITIVE_GEN_IDXS[i]].sAmount += presetInfo[ADDITIVE_GEN_IDXS[i]].sAmount;
    }

    presetIdx[pHead].emplace_back(SampleInfo{
        {
            instInfo[Keynum].sAmount == -1 ? std::max(presetInfo[KeyRange].lowByte, instInfo[KeyRange].lowByte) : static_cast<uint8_t>(instInfo[Keynum].sAmount),
            instInfo[Velocity].sAmount == -1 ? std::max(presetInfo[VelRange].lowByte, instInfo[VelRange].lowByte) : static_cast<uint8_t>(instInfo[Velocity].sAmount),
            instInfo[Keynum].sAmount == -1 ? std::min(presetInfo[KeyRange].highByte, instInfo[KeyRange].highByte) : static_cast<uint8_t>(instInfo[Keynum].sAmount),
            instInfo[Velocity].sAmount == -1 ? std::min(presetInfo[VelRange].highByte, instInfo[VelRange].highByte) : static_cast<uint8_t>(instInfo[Velocity].sAmount)
        }, {
            smplInfo.originalPitch,
            static_cast<LoopMode>(instInfo[sf_internal::GeneratorType::SampleModes].sAmount),
            instInfo[ScaleTuning].uAmount == 100,

            smplInfo.sampleRate,
            smplInfo.startOffset + std::max(instInfo[StartAddrsOffset].sAmount + instInfo[StartAddrsCoarseOffset].sAmount * 32768, 0) * (sf.version() == 2),
            smplInfo.endOffset + std::min(instInfo[EndAddrsOffset].sAmount + instInfo[EndAddrsCoarseOffset].sAmount * 32768, 0) * (sf.version() == 2),
            smplInfo.startLoop + std::max(instInfo[StartloopAddrsOffset].sAmount + instInfo[StartloopAddrsCoarseOffset].sAmount * 32768, 0) * (sf.version() == 2),
            smplInfo.endLoop + std::min(instInfo[EndloopAddrsOffset].sAmount + instInfo[EndloopAddrsCoarseOffset].sAmount * 32768, 0) * (sf.version() == 2),

            std::clamp(static_cast<float>(instInfo[Pan].sAmount) / 100.f, -0.5f, 0.5f),
            // Not sure the unit of InitialAttenuation, the standard seems to be wrong.
            // std::clamp(db_to_amplitude(-static_cast<float>(instInfo[InitialAttenuation].sAmount) / 25.f), db_to_amplitude(-144.f), 1.f),
            std::clamp(1.f - static_cast<float>(instInfo[InitialAttenuation].sAmount) / 1000.f, 0.f, 1.f),

            cent_to_tune(std::clamp(static_cast<float>(smplInfo.pitchCorrection + instInfo[FineTune].sAmount) + static_cast<float>(instInfo[CoarseTune].sAmount) * 100.f, -120.f, 120.f)),

            std::clamp(abscent_to_hz(instInfo[InitialFilterFc].sAmount), 20.f, 40000.f),
            std::clamp(cb_to_amplitude(instInfo[InitialFilterQ].sAmount), cb_to_amplitude(0.f), cb_to_amplitude(960.f)),

            std::clamp(timecents_to_s(instInfo[DelayVolEnv].sAmount), 0.001f, 20.f),
            std::clamp(timecents_to_s(instInfo[AttackVolEnv].sAmount), 0.001f, 100.f),
            std::clamp(timecents_to_s(instInfo[HoldVolEnv].sAmount), 0.001f, 20.f),
            std::clamp(timecents_to_s(instInfo[DecayVolEnv].sAmount), 0.001f, 100.f),
            std::clamp(1.f - static_cast<float>(instInfo[SustainVolEnv].sAmount) / 1000.f, 0.f, 1.f),
            std::clamp(timecents_to_s(instInfo[ReleaseVolEnv].sAmount), 0.001f, 100.f),

            std::clamp(timecents_to_s(instInfo[DelayModEnv].sAmount), 0.001f, 20.f),
            std::clamp(timecents_to_s(instInfo[AttackModEnv].sAmount), 0.001f, 100.f),
            std::clamp(timecents_to_s(instInfo[HoldModEnv].sAmount), 0.001f, 20.f),
            std::clamp(timecents_to_s(instInfo[DecayModEnv].sAmount), 0.001f, 100.f),
            std::clamp(1.f - static_cast<float>(instInfo[SustainModEnv].sAmount) / 1000.f, 0.f, 1.f),
            std::clamp(timecents_to_s(instInfo[ReleaseModEnv].sAmount), 0.001f, 100.f),
        }
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

    // TODO: Optimize sample searching
    for(int i = 0; i < resPreset->second.size(); ++i) {
        const auto &sampleInfo = resPreset->second[i];

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

    auto cachedSample = sampleCache.find(pitch | (sampleAttr.startOffset << 8));
    if(cachedSample != sampleCache.end())
        return cachedSample->second;

    AudioData sample = sf.sample(
        sampleAttr.startOffset,
        sampleAttr.endOffset - sampleAttr.startOffset,
        sampleAttr.sampleRate,
        sampleRate,
        quality);

    // TODO: Use better pitch shifting algorithm
    float shiftRatio = 1.f;
    if(sampleAttr.scaleTuning || (sampleAttr.pitch != pitch || sampleAttr.tune)) {
        shiftRatio = pitch_to_hz(sampleAttr.pitch) / pitch_to_hz(pitch) / sampleAttr.tune;
        sample = resample_mono(
            sample,
            shiftRatio,
            0);  // Using linear interpolation for now
    }

    sample *= sampleAttr.attenuation;

    Sample rawSample {
        sampleAttr,
        sample
    };

    shiftRatio *= static_cast<float>(sampleRate) / static_cast<float>(sampleAttr.sampleRate);
    if(sf.version() == 2) {
        rawSample.attr.startLoop -= rawSample.attr.startOffset;
        rawSample.attr.endLoop -= rawSample.attr.startOffset;
    }

    rawSample.attr.startLoop *= shiftRatio;
    rawSample.attr.endLoop *= shiftRatio;
    rawSample.attr.sampleRate = sampleRate;
    rawSample.attr.pitch = pitch;

    sampleCache[pitch | (sampleAttr.startOffset << 8)] = rawSample;

    return rawSample;
};

inline AudioData loop(const Sample &rawSample, uint32_t noteDurationFrames) {
    AudioData sample;
    if(rawSample.attr.loopMode == sf_internal::LoopMode::NoLoop ||
        rawSample.attr.loopMode == sf_internal::LoopMode::Unused ||
        noteDurationFrames <= rawSample.attr.endLoop) {

        if(noteDurationFrames <= rawSample.audio.cols())
            sample = rawSample.audio.leftCols(noteDurationFrames);
        else {
            noteDurationFrames = rawSample.audio.cols();
            sample = rawSample.audio;
        }
    } else if(rawSample.attr.loopMode == sf_internal::LoopMode::Coutinuous) {
        sample = Eigen::ArrayXXf::Zero(1, noteDurationFrames);
        uint32_t loopLength = rawSample.attr.endLoop - rawSample.attr.startLoop;
        uint32_t curOffset = rawSample.attr.endLoop;

        sample.leftCols(curOffset) = rawSample.audio.leftCols(curOffset);
        noteDurationFrames -= curOffset;

        while(noteDurationFrames > loopLength) {
            sample.middleCols(curOffset, loopLength) = rawSample.audio.middleCols(rawSample.attr.startLoop, loopLength);

            noteDurationFrames -= loopLength;
            curOffset += loopLength;
        }
        sample.rightCols(sample.cols() - curOffset) = rawSample.audio.middleCols(rawSample.attr.startLoop, sample.cols() - curOffset);
    } else if(rawSample.attr.loopMode == sf_internal::LoopMode::ToEnd) {
        sample = Eigen::ArrayXXf::Zero(1, noteDurationFrames);
        uint32_t loopLength = rawSample.attr.endLoop - rawSample.attr.startLoop;
        uint32_t curOffset = rawSample.attr.endLoop;

        sample.leftCols(curOffset) = rawSample.audio.leftCols(curOffset);
        noteDurationFrames -= curOffset;

        while(noteDurationFrames > rawSample.audio.cols() - rawSample.attr.startLoop) {
            sample.middleCols(curOffset, loopLength) = rawSample.audio.middleCols(rawSample.attr.startLoop, loopLength);
            noteDurationFrames -= loopLength;
            curOffset += loopLength;
        }
        sample.rightCols(noteDurationFrames) = rawSample.audio.middleCols(rawSample.attr.startLoop, noteDurationFrames);
    }

    return sample;
}

const AudioData PrestoSoundFont::build_sample(const SampleAttribute &attr, uint8_t pitch, uint8_t velocity, uint32_t durationFrames) {
    const Sample &rawSample = get_raw_sample(attr, pitch);

    Envelope velEnv(
        attr.loopMode,
        attr.delayVol,
        attr.attackVol,
        attr.holdVol,
        attr.decayVol,
        attr.sustainVol,
        attr.releaseVol,
        sampleRate,
        durationFrames);
    Envelope modEnv(
        attr.loopMode,
        attr.delayMod,
        attr.attackMod,
        attr.holdMod,
        attr.decayMod,
        attr.sustainMod,
        attr.releaseMod,
        sampleRate,
        durationFrames);
    LowPassFilter filter(
        attr.filterFc,
        attr.filterQ,
        sampleRate);

    uint32_t noteDurationFrames = velEnv.noteDurationFrames;

    // Processing loop
    AudioData sample = loop(rawSample, noteDurationFrames);

    // Processing LPF
    filter.process(sample);

    // Processing volume envelope
    sample.row(0) *= velEnv(sample.cols());

    return sample;
};

const AudioData PrestoSoundFont::build_note(uint8_t preset, uint8_t bank, uint8_t pitch, uint8_t velocity, uint32_t durationFrames, bool stereo) {
    const SampleInfoPack sampleInfos = get_sample_info(preset, bank, pitch, velocity);

    uint32_t maxFrames = 0;
    std::vector<AudioData> samples;
    samples.reserve(sampleInfos.size());
    for(auto sampleInfo : sampleInfos) {
        AudioData thisSample = build_sample(*sampleInfo, pitch, velocity, durationFrames);

        if(thisSample.cols() > maxFrames)
            maxFrames = thisSample.cols();

        samples.emplace_back(thisSample);
    }

    AudioData outputSample = Eigen::ArrayXXf::Zero(stereo ? 2 : 1, maxFrames);
    for(int i = 0; i < sampleInfos.size(); i++) {
        AudioData &sample = samples[i];
        if(stereo) {
            outputSample.row(0).leftCols(sample.cols()) += sample.row(0) * (0.5 - sampleInfos[i]->pan);
            outputSample.row(1).leftCols(sample.cols()) += sample.row(0) * (0.5 + sampleInfos[i]->pan);
        } else {
            outputSample.leftCols(sample.cols()) += sample;
        }
    }

    return outputSample;
};

}
