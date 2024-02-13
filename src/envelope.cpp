#include "prestosynth/envelope.h"

#include <Eigen/Core>

namespace psynth {

VolEnvelope::VolEnvelope(const SampleAttribute &attr, uint32_t sampleRate, uint32_t durationFrames) {
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

void VolEnvelope::process(AudioData &sample) const {
    // Delay
    if(this->delayFrames)
        sample.leftCols(this->attackStart) = 0;

    // Attack
    if(this->attackFrames)
        sample.middleCols(this->attackStart, this->attackFrames).row(0) *= Eigen::ArrayXf::LinSpaced(this->attackFrames, 0, this->holdLevel);

    // Hold
    if(this->holdFrames)
        sample.middleCols(this->holdStart, this->holdFrames).row(0) *= this->holdLevel;

    // Decay
    if(this->decayFrames)
        sample.middleCols(this->decayStart, this->decayFrames).row(0) *= Eigen::ArrayXf::LinSpaced(this->decayFrames, this->holdLevel, this->sustainLevel);

    // Sustain
    if(this->sustainFrames)
        sample.middleCols(this->sustainStart, this->sustainFrames) *= sustainLevel;

    // Release
    if(this->releaseFrames)
        sample.rightCols(this->releaseFrames).row(0) *= Eigen::ArrayXf::LinSpaced(this->releaseFrames, this->releaseLevel, 0.f);
};

}