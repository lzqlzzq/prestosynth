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

AudioData SoundFont::sample(const shdrData &sampleInfo, uint32_t targetSampleRate, uint8_t quality) {
    AudioData sampleData;

    if(_version == 2) {
        sampleData.resize(sampleInfo.len());

        // TODO: sm24
        src_short_to_float_array(
            reinterpret_cast<const short*>(smplHandler + sampleInfo.startOffset),
            sampleData.data(),
            sampleInfo.len());
    }
    else {
        int chan, samplerate;
        short *output;

        size_t sampleNum = stb_vorbis_decode_memory(
            smplHandler + sampleInfo.startOffset,
            sampleInfo.len(), &chan, &samplerate, &output);

        if(sampleNum == -1)
            throw std::ios_base::failure("Cannot decode sample!");

        sampleData.resize(sampleNum);
        src_short_to_float_array(
            output,
            sampleData.data(),
            sampleNum);
    }

    if(targetSampleRate == sampleInfo.sampleRate)
        return sampleData;

    SRC_DATA srcInfo;
    srcInfo.src_ratio = static_cast<double>(targetSampleRate) / static_cast<double>(sampleInfo.sampleRate);
    srcInfo.input_frames = sampleData.size();
    srcInfo.output_frames = sampleData.size() * srcInfo.src_ratio;
    AudioData resampledData(srcInfo.output_frames);
    srcInfo.data_in = sampleData.data();
    srcInfo.data_out = resampledData.data();

    src_simple(&srcInfo, 4 - quality, 1);

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

inline void PrestoSoundFont::handle_gen(SampleInfo &sInfo, const sf_internal::Generator &gen) {
    switch(gen.genOper) {
        case sf_internal::GeneratorType::KeyRange: {
            sInfo.head.pitchLow = gen.lowByte;
            sInfo.head.pitchHigh = gen.highByte;
            break;
        };
        case sf_internal::GeneratorType::VelRange: {
            sInfo.head.velLow = gen.lowByte;
            sInfo.head.velHigh = gen.highByte;
            break;
        };
        case sf_internal::GeneratorType::Pan: {
            sInfo.attr.pan = static_cast<float>(gen.sAmount) / 500.;
            break;
        };
        case sf_internal::GeneratorType::InitialAttenuation: {
            sInfo.attr.attenuation = cb_to_amplitude(static_cast<float>(-gen.sAmount));
            break;
        };
        case sf_internal::GeneratorType::DelayVolEnv: {
            sInfo.attr.delayVol = timecents_to_ms(gen.sAmount);
            break;
        };
        case sf_internal::GeneratorType::AttackVolEnv: {
            sInfo.attr.attackVol = timecents_to_ms(gen.sAmount);
            break;
        };
        case sf_internal::GeneratorType::HoldVolEnv: {
            sInfo.attr.holdVol = timecents_to_ms(gen.sAmount);
            break;
        };
        case sf_internal::GeneratorType::DecayVolEnv: {
            sInfo.attr.decayVol = timecents_to_ms(gen.sAmount);
            break;
        };
        case sf_internal::GeneratorType::SustainVolEnv: {
            sInfo.attr.sustainVol = cb_to_amplitude(static_cast<float>(-gen.sAmount));
            break;
        };
        case sf_internal::GeneratorType::ReleaseVolEnv: {
            sInfo.attr.releaseVol = timecents_to_ms(gen.sAmount);
            break;
        };
    }
};

inline void PrestoSoundFont::handle_smpl(SampleInfo instInfo, const PresetHead pHead, uint16_t smplIdx) {
    SampleInfo sInfo = instInfo;
    const auto &smplInfo = sf.shdr(smplIdx);

    sInfo.attr.pitch = smplInfo.originalPitch;
    sInfo.attr.sampleRate = smplInfo.sampleRate;
    sInfo.attr.startOffset = smplInfo.startOffset;
    sInfo.attr.endOffset = smplInfo.endOffset;
    sInfo.attr.startLoop = smplInfo.startLoop;
    sInfo.attr.endLoop = smplInfo.endLoop;

    presetIdx[pHead].push_back(sInfo);
}

inline void PrestoSoundFont::handle_inst(SampleInfo presetInfo, const PresetHead pHead, uint16_t instIdx) {
    SampleInfo iGlobalInfo = presetInfo;

    const auto &thisInst = sf.inst(instIdx);
    const auto &nextInst = sf.inst(instIdx + 1);
    for(uint32_t iBagIdx = thisInst.bagIdx; iBagIdx < nextInst.bagIdx; iBagIdx++) {
        SampleInfo iZoneInfo = iGlobalInfo;
        bool iGlobal = true;

        for(uint16_t genIdx = sf.ibag(iBagIdx).genIdx; genIdx < sf.ibag(iBagIdx + 1).genIdx; genIdx++) {
            const auto &iGen = sf.igen(genIdx);
            handle_gen(iZoneInfo, iGen);

            if(iGen.genOper == sf_internal::GeneratorType::SampleID) {
                iGlobal = false;
                handle_smpl(iZoneInfo, pHead, iGen.uAmount);

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

        SampleInfo pGlobalInfo;
        bool pGlobal = true;
        for(uint32_t pBagIdx = thisPreset.bagIdx; pBagIdx < nextPreset.bagIdx; ++pBagIdx) {
            SampleInfo pZoneInfo = pGlobalInfo;

            for(uint16_t genIdx = sf.pbag(pBagIdx).genIdx; genIdx < sf.pbag(pBagIdx + 1).genIdx; genIdx++) {
                const auto &pGen = sf.pgen(genIdx);
                handle_gen(pZoneInfo, pGen);

                if(pGen.genOper == sf_internal::GeneratorType::Instrument) {
                    pGlobal = false;
                    handle_inst(pZoneInfo, pHead, pGen.uAmount);

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

PrestoSoundFont::PrestoSoundFont(const std::string &filepath, uint32_t sampleRate, bool stereo):
    sf(sf_internal::SoundFont(filepath)), sampleRate(sampleRate), stereo(stereo) {
    handle_phdr();
};

}
