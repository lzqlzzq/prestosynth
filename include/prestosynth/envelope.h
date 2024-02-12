#ifndef ENVELOPE_H
#define ENVELOPE_H

#include "prestosynth/soundfont.h"
#include "prestosynth/util/audio_util.h"

namespace psynth {

class VolEnvelope {
public:
    uint32_t noteDurationFrames;
    uint32_t durationFrames;

    uint32_t delayFrames = 0;

    uint32_t attackStart;
    uint32_t attackFrames = 0;

    uint32_t holdStart;
    uint32_t holdFrames = 0;
    float holdLevel = 1.;

    uint32_t decayStart;
    uint32_t decayFrames = 0;

    uint32_t sustainStart;
    uint32_t sustainFrames = 0;
    float sustainLevel = 1.;

    uint32_t releaseStart;
    uint32_t releaseFrames = 0;
    float releaseLevel = 0.f;

    VolEnvelope(const SampleAttribute &attr, uint32_t sampleRate, uint32_t durationFrames);

    void process(AudioData &sample) const;
};

}

#endif //ENVELOPE_H
