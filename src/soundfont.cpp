#include <algorithm>
#include "samplerate.h"
#include "soundfont.h"
#include "byte_util.h"
#include "math_util.h"
#include "stb_vorbis.c"

#include <iostream>

namespace prestosynth {

namespace sf_internal {

uint8_t* SoundFont::cursor(size_t offset) {
    return const_cast<uint8_t*>(handler.begin()) + offset;
};

void SoundFont::read_INFO_chunk(size_t offset, size_t chunkSize) {
    size_t endOffset = offset + chunkSize;
    while(!_version) {
        size_t chunkSize = read_le_bytes<uint32_t>(cursor(offset + 4));
        if(offset + chunkSize > endOffset)
            throw std::ios_base::failure("Unexcepted EOF in INFO chunk!");

        if(read_string_bytes(cursor(offset), 4) == ifil_CHUNKID) {
            _version = read_le_bytes<uint32_t>(cursor(offset + 8));
            break;
        }

        offset + chunkSize + 8;
    }
    if(_version != 2 && _version != 3)
        throw std::ios_base::failure("Only sf2 or sf3 supported!");
};

void SoundFont::read_sdta_chunk(size_t offset, size_t chunkSize) {
    size_t endOffset = offset + chunkSize;
    while(!smplHandler) {
        size_t chunkSize = read_le_bytes<uint32_t>(cursor(offset + 4));
        if(offset + chunkSize > endOffset)
            throw std::ios_base::failure("Unexcepted EOF in sdta chunk!");

        if(read_string_bytes(cursor(offset), 4) == smpl_CHUNKID) {
            smplHandler = cursor(offset + 8);
            smplSize = read_le_bytes<uint32_t>(cursor(offset + 4));
            break;
        }
        // TODO: sm24

        offset + chunkSize + 8;
    }
};

void SoundFont::read_pdta_chunk(size_t offset, size_t chunkSize) {
    size_t endOffset = offset + chunkSize;
    while(!(
            #define SF_CHUNK_TYPE(name) name##Num &&
            PDTA_SUB_CHUNK_TYPES
            #undef SF_CHUNK_TYPE
            true)) {
        size_t chunkSize = read_le_bytes<uint32_t>(cursor(offset + 4));

        if(offset + chunkSize > endOffset)
            throw std::ios_base::failure("Unexcepted EOF in pdta chunk!");

        #define SF_CHUNK_TYPE(name)                                                        \
        if(read_string_bytes(cursor(offset), 4) == name##_CHUNKID) {                       \
            if(chunkSize % sizeof(name##Data))                                             \
                throw std::ios_base::failure("Not valid " + name##_CHUNKID + " chunk!");   \
            name##Handler = reinterpret_cast<name##Data*>(cursor(offset + 8));           \
            name##Num = read_le_bytes<uint32_t>(cursor(offset + 4)) / sizeof(name##Data);         \
        }
        PDTA_SUB_CHUNK_TYPES
        #undef SF_CHUNK_TYPE

        offset += chunkSize + 8;
    }
};

SoundFont::SoundFont(const std::string &filepath) {
    std::error_code err;
    handler.map(filepath, err);

    if(err)
        throw std::ios_base::failure(std::string("Error mapping file: ") + filepath + std::string("\n") + err.message());

    if(read_string_bytes(cursor(0), 4) != RIFF_CHUNKID)
        throw std::ios_base::failure("Not a valid RIFF file!");

    size_t fileSize = read_le_bytes<uint32_t>(cursor(4));

    if(fileSize > handler.size() - 8)
        throw std::ios_base::failure("Unexcepted EOF!");

    if(read_string_bytes(cursor(8), 4) != sfbk_CHUNKID)
        throw std::ios_base::failure("Not a valid sfbk file!");

    size_t offset = 12;

#define SF_CHUNK_TYPE(name)                              \
    bool name##Processed = false;
SF_CHUNK_TYPES
#undef SF_CHUNK_TYPE

    while(!(
    #define SF_CHUNK_TYPE(name) \
        name##Processed &&
    SF_CHUNK_TYPES
    #undef SF_CHUNK_TYPE
    true)) {
        if(read_string_bytes(cursor(offset), 4) != LIST_CHUNKID)
            throw std::ios_base::failure("LIST chunk excepted!");

        size_t chunkSize = read_le_bytes<uint32_t>(cursor(offset + 4));

        if(offset + chunkSize > fileSize)
            throw std::ios_base::failure("Unexcepted EOF!");

    #define SF_CHUNK_TYPE(name) \
        if(read_string_bytes(cursor(offset + 8), 4) == name##_CHUNKID) { \
            read_##name##_chunk(offset + 12, chunkSize); \
            name##Processed = true; \
        }
    SF_CHUNK_TYPES
    #undef SF_CHUNK_TYPE

        offset += chunkSize + 8;
    }
};

SoundFont::~SoundFont() {
    handler.unmap();
};

uint16_t SoundFont::version() const {
    return _version;
};

AudioData SoundFont::sample(uint32_t startOffset, uint32_t length, uint16_t originalSampleRate, uint32_t targetSampleRate, uint8_t quality) {
    std::lock_guard<std::mutex> lk(cacheMtx);

    auto cachedSample = sampleCache.find(startOffset);
    if(cachedSample != sampleCache.end())
        return cachedSample->second;

    AudioData sampleData;

    if(_version == 2) {
        sampleData = zero_audio_data(1, length);

        // TODO: sm24
        src_short_to_float_array(
            reinterpret_cast<const short*>(smplHandler + startOffset),
            sampleData.data(),
            length);
    }
    else {
        int chan, samplerate;
        short *output;

        size_t sampleNum = stb_vorbis_decode_memory(
            smplHandler + startOffset,
            length, &chan, &samplerate, &output);

        if(sampleNum == -1)
            throw std::ios_base::failure("Cannot decode sample!");

        sampleData = zero_audio_data(1, sampleNum);
        src_short_to_float_array(
            output,
            sampleData.data(),
            sampleNum);
    }

    if(targetSampleRate == originalSampleRate)
        return sampleData;

    AudioData resampledData = resample_mono(sampleData, static_cast<double>(targetSampleRate) / static_cast<double>(originalSampleRate), quality);

    sampleCache[startOffset] = resampledData;
    return resampledData;
}

#define SF_CHUNK_TYPE(name)                                                               \
name##Data SoundFont::name(size_t index) const {                                          \
    if(index > name##Num)                                                                 \
        std::out_of_range("Index is out of range!");                                      \
    return *(name##Handler + index);                                                      \
};                                                                                        \
                                                                                          \
size_t SoundFont::name##_num() const {                                                    \
    return name##Num;                                                                     \
};
PDTA_SUB_CHUNK_TYPES
#undef SF_CHUNK_TYPE

}

VolEnvelope::VolEnvelope(const SampleAttribute &attr, uint32_t sampleRate, float duration) {
    durationFrames = s_to_frames(duration, sampleRate);
    releaseFrames = s_to_frames(attr.releaseVol, sampleRate);
    noteDurationFrames = durationFrames + releaseFrames;
    
    uint32_t remainFrames = durationFrames;
    uint32_t curPosition = 0;

    delayFrames = attr.delayVol <= 0.001 ? 0 : s_to_frames(attr.delayVol, sampleRate);
    if(remainFrames <= delayFrames) {
        delayFrames = remainFrames;
        attackStart = curPosition + remainFrames;

        goto release;
    }

    remainFrames -= delayFrames;
    curPosition += delayFrames;

    attackStart = curPosition;
    attackFrames = attr.attackVol <= 0.001 ? 0 :s_to_frames(attr.attackVol, sampleRate);
    if(remainFrames <= attackFrames) {
        releaseLevel = static_cast<float>(remainFrames) / static_cast<float>(attackFrames);
        holdLevel = releaseLevel;
        attackFrames = remainFrames;
        holdStart = curPosition + remainFrames;

        goto release;
    }

    remainFrames -= attackFrames;
    curPosition += attackFrames;

    holdStart = curPosition;
    holdFrames = attr.holdVol <= 0.001 ? 0 : s_to_frames(attr.holdVol, sampleRate);
    if(remainFrames <= holdFrames) {
        releaseLevel = 1.f;
        holdFrames = remainFrames;
        decayStart = curPosition + remainFrames;

        goto release;
    }

    remainFrames -= holdFrames;
    curPosition += holdFrames;

    decayStart = curPosition;
    decayFrames = attr.delayVol <= 0.001 ? 0 : s_to_frames(attr.decayVol, sampleRate);
    if(remainFrames <= decayFrames) {
        releaseLevel = 1.f - static_cast<float>(remainFrames) / static_cast<float>(decayFrames);
        sustainLevel = releaseLevel;
        decayFrames = remainFrames;
        sustainStart = curPosition + remainFrames;

        goto release;
    }

    remainFrames -= decayFrames;
    curPosition += decayFrames;

    releaseLevel = sustainLevel;
    sustainStart = curPosition;
    sustainFrames = remainFrames;

release:
    releaseStart = curPosition + remainFrames;
};

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
            smplInfo.startOffset,
            smplInfo.endOffset,
            smplInfo.startLoop,
            smplInfo.endLoop,

            std::clamp(static_cast<float>(instInfo[Pan].sAmount) / 100.f, -0.5f, 0.5f),
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
    // for(auto pIdx = 2; pIdx < 3; ++pIdx) {
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

PrestoSoundFont::PrestoSoundFont(const std::string &filepath, uint32_t sampleRate, bool stereo, uint8_t quality):
    sf(sf_internal::SoundFont(filepath)), sampleRate(sampleRate), stereo(stereo), quality(quality) {
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

    auto cachedSample = sampleCache.find(pitch + (sampleAttr.startOffset << 8));
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
    rawSample.attr.startLoop *= shiftRatio;
    rawSample.attr.endLoop *= shiftRatio;
    rawSample.attr.sampleRate = sampleRate;
    rawSample.attr.pitch = pitch;

    sampleCache[pitch + (sampleAttr.startOffset << 8)] = rawSample;

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
    // Delay
    if(velEnv.delayFrames)
        xt::view(sample, xt::all(), xt::range(_, velEnv.attackStart)) *= 0;
    // Attack
    if(velEnv.attackFrames)
        xt::view(sample, xt::all(), xt::range(velEnv.attackStart, velEnv.holdStart)) *= xt::linspace<audio_t>(0., velEnv.holdLevel, velEnv.attackFrames);

    // Decay
    if(velEnv.decayFrames)
        xt::view(sample, xt::all(), xt::range(velEnv.decayStart, velEnv.sustainStart)) *= xt::linspace<audio_t>(1., velEnv.sustainLevel, velEnv.decayFrames);
    
    // Sustain
    if(velEnv.sustainFrames)
        xt::view(sample, xt::all(), xt::range(velEnv.sustainStart, velEnv.releaseStart)) *= velEnv.sustainLevel;

    // Release
    if(velEnv.releaseFrames)
        xt::view(sample, xt::all(), xt::range(velEnv.releaseStart, _)) *= xt::linspace<audio_t>(velEnv.releaseLevel, 0, velEnv.releaseFrames);

    return sample;
};

const AudioData PrestoSoundFont::build_note(uint8_t preset, uint8_t bank, uint8_t pitch, uint8_t velocity, float duration) {
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
