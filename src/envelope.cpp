#include "prestosynth/envelope.h"

#include <Eigen/Core>

namespace psynth {

Envelope::Envelope(
    sf_internal::LoopMode loopMode,
    float delay,
    float attack,
    float hold,
    float decay,
    float sustain,
    float release,
    float sampleRate,
    uint32_t durationFrames) {
    if((loopMode == sf_internal::LoopMode::NoLoop ||
        loopMode == sf_internal::LoopMode::Unused) &&
        (!sustain)) {
        uint32_t ahFrames = s_to_frames(delay + attack + hold, sampleRate) + 1;
        if(durationFrames > ahFrames)
            durationFrames = ahFrames;
    }

    releaseFrames = s_to_frames(release, sampleRate);
    noteDurationFrames = durationFrames + releaseFrames;

    uint32_t remainFrames = durationFrames;
    uint32_t curPosition = 0;

    delayFrames = delay <= 0.001 ? 0 : s_to_frames(delay, sampleRate);
    if(remainFrames <= delayFrames) {
        delayFrames = remainFrames;
        attackStart = curPosition + remainFrames;

        goto release;
    }

    remainFrames -= delayFrames;
    curPosition += delayFrames;

    attackStart = curPosition;
    attackFrames = attack <= 0.001 ? 0 :s_to_frames(attack, sampleRate);
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
    holdFrames = hold <= 0.001 ? 0 : s_to_frames(hold, sampleRate);
    if(remainFrames <= holdFrames) {
        releaseLevel = 1.f;
        holdFrames = remainFrames;
        decayStart = curPosition + remainFrames;

        goto release;
    }

    remainFrames -= holdFrames;
    curPosition += holdFrames;

    decayStart = curPosition;
    decayFrames = delay <= 0.001 ? 0 : s_to_frames(decay, sampleRate);
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

    // Sustain Penalty
    // Following https://github.com/schellingb/TinySoundFont/blob/master/tsf.h#L1057
    uint32_t originalReleaseFrames = releaseFrames;

    if(releaseLevel && releaseLevel < 1.f) {
        releaseFrames *= -gcem::log10(-(releaseLevel - 1.f)) / 4.f;
        noteDurationFrames -= originalReleaseFrames - releaseFrames;
    }
};

AudioData Envelope::operator()(uint32_t length) const {
    AudioData envelope(1, length);
    uint32_t remainFrames = length;
    // Delay
    if(this->delayFrames && remainFrames) {
        if(remainFrames < this->delayFrames) {
            envelope.leftCols(remainFrames) = 0;
            return envelope;
        } else
            envelope.leftCols(this->attackStart) = 0;
    }
    remainFrames -= this->delayFrames;

    // Attack
    if(this->attackFrames && remainFrames) {
        if(remainFrames < this->attackFrames) {
            envelope.middleCols(this->attackStart, remainFrames).row(0) = Eigen::ArrayXf::LinSpaced(remainFrames, 0, this->holdLevel * (static_cast<float>(remainFrames) / static_cast<float>(this->attackFrames)));
            return envelope;
        } else
            envelope.middleCols(this->attackStart, this->attackFrames).row(0) = Eigen::ArrayXf::LinSpaced(this->attackFrames, 0, this->holdLevel);
    }
    remainFrames -= this->attackFrames;

    // Hold
    if(this->holdFrames && remainFrames) {
        if(remainFrames < this->holdFrames) {
            envelope.middleCols(this->holdStart, remainFrames).row(0) = this->holdLevel;
            return envelope;
        } else
            envelope.middleCols(this->holdStart, this->holdFrames).row(0) = this->holdLevel;
    }
    remainFrames -= this->holdFrames;

    // Decay
    if(this->decayFrames && remainFrames) {
        if(remainFrames < this->decayFrames)  {
            envelope.middleCols(this->decayStart, remainFrames).row(0) = Eigen::ArrayXf::LinSpaced(remainFrames, this->holdLevel, this->sustainLevel + (this->holdLevel - this->sustainLevel) * (static_cast<float>(remainFrames) / static_cast<float>(this->decayFrames)));
            return envelope;
        } else
            envelope.middleCols(this->decayStart, this->decayFrames).row(0) = Eigen::ArrayXf::LinSpaced(this->decayFrames, this->holdLevel, this->sustainLevel);
    }
    remainFrames -= this->decayFrames;

    // Sustain
    if(this->sustainFrames && remainFrames) {
        if(remainFrames < this->sustainFrames) {
            envelope.middleCols(this->sustainStart, remainFrames) = sustainLevel;
            return envelope;
        } else
            envelope.middleCols(this->sustainStart, this->sustainFrames) = sustainLevel;
    }
    remainFrames -= this->sustainFrames;

    // Release
    if(this->releaseFrames && remainFrames) {
        if(remainFrames < this->releaseFrames) {
            envelope.rightCols(remainFrames).row(0) = Eigen::ArrayXf::LinSpaced(this->releaseFrames, this->releaseLevel, this->releaseLevel * (static_cast<float>(remainFrames) / static_cast<float>(this->releaseFrames)));
        }
        else
            envelope.rightCols(this->releaseFrames).row(0) = Eigen::ArrayXf::LinSpaced(this->releaseFrames, this->releaseLevel, 0.f);
    }

    return envelope;
};

}