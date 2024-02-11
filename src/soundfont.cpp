#include <algorithm>

#include "xtensor/xpad.hpp"
#include "soundfont.h"
#include "envelope.h"
#include "util/math_util.h"

namespace psynth {

using namespace xt::placeholders;

inline void PrestoSoundFont::handle_smpl(sf_internal::GeneratorPack presetInfo, sf_internal::GeneratorPack instInfo, const PresetHead pHead, uint16_t smplIdx) {
    using namespace sf_internal;
    constexpr std::array<uint16_t, 38> ADDITIVE_GEN_IDXS {5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 48, 51, 52};

    const auto &smplInfo = sf.shdr(smplIdx);

    for(int i = 0; i < ADDITIVE_GEN_IDXS.size(); ++i) {
        if(presetInfo[ADDITIVE_GEN_IDXS[i]].sAmount != defaultGenPack[ADDITIVE_GEN_IDXS[i]].sAmount)
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
            static_cast<uint8_t>(instInfo[sf_internal::GeneratorType::SampleModes].sAmount),

            smplInfo.sampleRate,
            smplInfo.startOffset + instInfo[StartAddrsOffset].sAmount + instInfo[StartAddrsCoarseOffset].sAmount * 32768,
            smplInfo.endOffset + instInfo[EndAddrsOffset].sAmount + instInfo[EndAddrsCoarseOffset].sAmount * 32768,
            smplInfo.startLoop + instInfo[StartloopAddrsOffset].sAmount + instInfo[StartloopAddrsCoarseOffset].sAmount * 32768,
            smplInfo.endLoop + instInfo[EndloopAddrsOffset].sAmount + instInfo[EndloopAddrsCoarseOffset].sAmount * 32768,

            std::clamp(static_cast<float>(instInfo[Pan].sAmount) / 100.f, -0.5f, 0.5f),
            // Not sure the unit of InitialAttenuation, the standard seems to be wrong.
            // std::clamp(db_to_amplitude(-static_cast<float>(instInfo[InitialAttenuation].sAmount) / 25.f), db_to_amplitude(-144.f), 1.f),
            std::clamp(1.f - static_cast<float>(instInfo[InitialAttenuation].sAmount) / 1000.f, 0.f, 1.f),
            std::clamp(timecents_to_s(instInfo[DelayVolEnv].sAmount), 0.001f, 20.f),
            std::clamp(timecents_to_s(instInfo[AttackVolEnv].sAmount), 0.001f, 100.f),
            std::clamp(timecents_to_s(instInfo[HoldVolEnv].sAmount), 0.001f, 20.f),
            std::clamp(timecents_to_s(instInfo[DecayVolEnv].sAmount), 0.001f, 100.f),
            std::clamp(1.f - static_cast<float>(instInfo[SustainVolEnv].sAmount) / 1000.f, 0.f, 1.f),
            std::clamp(timecents_to_s(instInfo[ReleaseVolEnv].sAmount), 0.001f, 100.f),
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

    AudioData monoSample = sf.sample(
        sampleAttr.startOffset,
        sampleAttr.endOffset - sampleAttr.startOffset,
        sampleAttr.sampleRate,
        sampleRate,
        quality);

    // TODO: Use better pitch shifting algorithm
    float shiftRatio = pitch_to_hz(sampleAttr.pitch) / pitch_to_hz(pitch);
    if(sampleAttr.pitch != pitch) {
        monoSample = resample_mono(
            monoSample,
            shiftRatio,
            0);  // Using linear interpolation for now
    }

    monoSample = monoSample * sampleAttr.attenuation;

    Sample rawSample {
        sampleAttr,
        monoSample
    };

    shiftRatio *= static_cast<float>(sampleRate) / static_cast<float>(sampleAttr.sampleRate);
    if(sf.version() == 2) {
        rawSample.attr.startLoop -= rawSample.attr.startOffset;
        rawSample.attr.endLoop -= rawSample.attr.startOffset;
        rawSample.attr.startLoop *= shiftRatio;
        rawSample.attr.endLoop *= shiftRatio;
    } else {
        rawSample.attr.startLoop *= shiftRatio;
        rawSample.attr.endLoop *= shiftRatio;
    }
    rawSample.attr.sampleRate = sampleRate;
    rawSample.attr.pitch = pitch;

    sampleCache[pitch | (sampleAttr.startOffset << 8)] = rawSample;

    return rawSample;
};

const AudioData PrestoSoundFont::build_sample(const SampleAttribute &sampleAttr, uint8_t pitch, uint8_t velocity, float duration) {
    const Sample &rawSample = get_raw_sample(sampleAttr, pitch);

    // TODO: Evaluate attenuation by velocity
    VolEnvelope velEnv(sampleAttr, sampleRate, duration);

    uint32_t noteDurationFrames = velEnv.noteDurationFrames;
    AudioData sample = empty_audio_data(1, noteDurationFrames);

    // Processing loop
    if(rawSample.attr.loopMode == sf_internal::LoopMode::NoLoop ||
        rawSample.attr.loopMode == sf_internal::LoopMode::Unused ||
        noteDurationFrames <= rawSample.attr.endLoop) {

        if(noteDurationFrames <= rawSample.audio.shape(1))
            sample = xt::view(rawSample.audio, xt::all(), xt::range(_, noteDurationFrames));
        else
            sample = xt::pad(rawSample.audio, {{0, 0}, {0, noteDurationFrames - rawSample.audio.shape(1)}});
    } else if(rawSample.attr.loopMode == sf_internal::LoopMode::Coutinuous) {
        uint32_t loopLength = rawSample.attr.endLoop - rawSample.attr.startLoop;
        uint32_t curOffset = rawSample.attr.endLoop;
        xt::view(sample, xt::all(), xt::range(_, curOffset)) = xt::view(rawSample.audio, xt::all(), xt::range(_, curOffset));
        noteDurationFrames -= curOffset;

        while(noteDurationFrames > loopLength) {
            xt::view(sample, xt::all(), xt::range(curOffset, curOffset + loopLength)) = xt::view(rawSample.audio, xt::all(), xt::range(rawSample.attr.startLoop, rawSample.attr.endLoop));
            noteDurationFrames -= loopLength;
            curOffset += loopLength;
        }
        xt::view(sample, xt::all(), xt::range(curOffset, _)) = xt::view(rawSample.audio, xt::all(), xt::range(rawSample.attr.startLoop, rawSample.attr.startLoop + noteDurationFrames));
    } else if(rawSample.attr.loopMode == sf_internal::LoopMode::ToEnd) {
        uint32_t loopLength = rawSample.attr.endLoop - rawSample.attr.startLoop;
        uint32_t curOffset = rawSample.attr.endLoop;

        xt::view(sample, xt::all(), xt::range(_, curOffset)) = xt::view(rawSample.audio, xt::all(), xt::range(_, curOffset));
        noteDurationFrames -= curOffset;

        while(noteDurationFrames > rawSample.audio.shape(1) - rawSample.attr.startLoop) {
            xt::view(sample, xt::all(), xt::range(curOffset, curOffset + loopLength)) = xt::view(rawSample.audio, xt::all(), xt::range(rawSample.attr.startLoop, rawSample.attr.endLoop));
            noteDurationFrames -= loopLength;
            curOffset += loopLength;
        }
        xt::view(sample, xt::all(), xt::range(curOffset, _)) = xt::view(rawSample.audio, xt::all(), xt::range(rawSample.attr.startLoop, rawSample.attr.startLoop + noteDurationFrames));
    }

    // Processing volume envelope
    velEnv.apply(sample);

    return sample;
};

const AudioData PrestoSoundFont::build_note(uint8_t preset, uint8_t bank, uint8_t pitch, uint8_t velocity, float duration, bool stereo) {
    const SampleInfoPack sampleInfos = get_sample_info(preset, bank, pitch, velocity);

    AudioData outputSample = zero_audio_data(stereo ? 2 : 1, 1);
    uint32_t maxFrames = 0;
    for(auto sampleInfo : sampleInfos) {
        AudioData thisSample = build_sample(*sampleInfo, pitch, velocity, duration);

        if(outputSample.shape(1) < thisSample.shape(1)) {
            std::swap(thisSample, outputSample);

            if(stereo)
                outputSample = xt::tile(outputSample, {2, 1});
        }

        if(stereo) {
            xt::view(outputSample, 0, xt::range(_, thisSample.shape(1))) += xt::row(thisSample, 0) * (0.5 - sampleInfo->pan);
            xt::view(outputSample, 1, xt::range(_, thisSample.shape(1))) += xt::row(thisSample, 0) * (0.5 + sampleInfo->pan);
        } else {
            xt::view(outputSample, xt::all(), xt::range(_, thisSample.shape(1))) += thisSample;
        }

    }

    return outputSample;
};

}
