#ifndef _SOUNDFONT_H
#define _SOUNDFONT_H

#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <mutex>
#include "prestosynth/soundfont_internal.h"
#include "prestosynth/util/audio_util.h"
#include "prestosynth/util/math_util.h"


namespace psynth {

union SampleHead {
    struct {
        uint8_t pitchLow = 0;
        uint8_t velLow = 0;
        uint8_t pitchHigh = 127;
        uint8_t velHigh = 127;
    };
    struct {
        uint16_t lowCode;
        uint16_t highCode;
    };
    uint32_t code;
};

struct SampleAttribute {
    uint8_t pitch = 0;
    uint8_t loopMode = 0;

    uint32_t sampleRate = 0;
    uint32_t startOffset = 0;
    uint32_t endOffset = 0;
    uint32_t startLoop = 0;
    uint32_t endLoop = 0;

    float pan = 0;
    float attenuation = 1.;
    float delayVol = timecents_to_s(-12000);
    float attackVol = timecents_to_s(-12000);
    float holdVol = timecents_to_s(-12000);
    float decayVol = timecents_to_s(-12000);
    float sustainVol = 1.;
    float releaseVol = timecents_to_s(-12000);
};

struct SampleInfo {
    SampleHead head;
    SampleAttribute attr;
};

typedef std::vector<const SampleAttribute*> SampleInfoPack;

struct Sample {
    SampleAttribute attr;
    AudioData audio;
};

// (Bank, Preset)
typedef uint16_t PresetHead;
typedef std::vector<SampleInfo> SampleInfos;
typedef std::map<PresetHead, SampleInfos> PresetIndex;

class PrestoSoundFont {
private:
    sf_internal::SoundFont sf;
    uint32_t sampleRate;
    uint8_t quality;
    PresetIndex presetIdx;

    std::mutex cacheMtx;
    std::map<uint64_t, Sample> sampleCache;

    void handle_smpl(sf_internal::GeneratorPack presetInfo, sf_internal::GeneratorPack instInfo, const PresetHead pHead, uint16_t smplIdx);
    void handle_inst(sf_internal::GeneratorPack presetInfo, const PresetHead pHead, uint16_t instIdx);
    void handle_phdr();

    const SampleInfoPack get_sample_info(uint8_t preset, uint8_t bank, uint8_t pitch, uint8_t velocity) const;
    const Sample get_raw_sample(const SampleAttribute &sampleAttr, uint8_t pitch);
    const AudioData build_sample(const SampleAttribute &sampleAttr, uint8_t pitch, uint8_t velocity, uint32_t durationFrames);

public:
    PrestoSoundFont(const std::string &filepath, uint32_t sampleRate, uint8_t quality);

    const AudioData build_note(uint8_t preset, uint8_t bank, uint8_t pitch, uint8_t velocity, uint32_t durationFrames, bool stereo);
};

}

#endif