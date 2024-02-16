#include "prestosynth/envelope.h"

#include <Eigen/Core>

namespace psynth {

void VolEnvelope::_handle_ahdsr_env(const SampleAttribute &attr, uint32_t sampleRate, uint32_t durationFrames) {
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

    // Sustain Penalty
    // Following https://github.com/schellingb/TinySoundFont/blob/master/tsf.h#L1057
    uint32_t originalReleaseFrames = releaseFrames;

    if(releaseLevel < 1.f) {
        releaseFrames *= -gcem::log10(-(releaseLevel - 1.f)) / 4.f;
        noteDurationFrames -= originalReleaseFrames - releaseFrames;
    }
};

void VolEnvelope::_handle_ahd_env(const SampleAttribute &attr, uint32_t sampleRate, uint32_t durationFrames) {
    sustainLevel = 0.f;

    decayFrames = s_to_frames(attr.decayVol, sampleRate);
    noteDurationFrames = durationFrames + decayFrames;

    uint32_t remainFrames = durationFrames;
    uint32_t curPosition = 0;

    delayFrames = attr.delayVol <= 0.001 ? 0 : s_to_frames(attr.delayVol, sampleRate);
    if(remainFrames <= delayFrames) {
        delayFrames = remainFrames;
        attackStart = curPosition + remainFrames;

        goto decay;
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

        goto decay;
    }

    remainFrames -= attackFrames;
    curPosition += attackFrames;

    holdStart = curPosition;
    holdFrames = attr.holdVol <= 0.001 ? 0 : s_to_frames(attr.holdVol, sampleRate);
    if(remainFrames <= holdFrames) {
        releaseLevel = 1.f;
        holdFrames = remainFrames;
        decayStart = curPosition + remainFrames;

        goto decay;
    }

    remainFrames -= holdFrames;
    curPosition += holdFrames;

decay:
    decayStart = curPosition + remainFrames;
};

VolEnvelope::VolEnvelope(const SampleAttribute &attr, uint32_t sampleRate, uint32_t durationFrames) {
    if(!attr.sustainVol)
        _handle_ahd_env(attr, sampleRate, durationFrames);
    else
        _handle_ahdsr_env(attr, sampleRate, durationFrames);
};

void VolEnvelope::process(AudioData &sample) const {
    uint32_t remainFrames = sample.cols();
    // Delay
    if(this->delayFrames && remainFrames) {
        if(remainFrames < this->delayFrames) {
            sample.leftCols(remainFrames) = 0;
            return ;
        } else
            sample.leftCols(this->attackStart) = 0;
    }
    remainFrames -= this->delayFrames;

    // Attack
    if(this->attackFrames && remainFrames) {
        if(remainFrames < this->attackFrames) {
            sample.middleCols(this->attackStart, remainFrames).row(0) *= Eigen::ArrayXf::LinSpaced(remainFrames, 0, this->holdLevel * (static_cast<float>(remainFrames) / static_cast<float>(this->attackFrames)));
            return ;
        } else
            sample.middleCols(this->attackStart, this->attackFrames).row(0) *= Eigen::ArrayXf::LinSpaced(this->attackFrames, 0, this->holdLevel);
    }
    remainFrames -= this->attackFrames;

    // Hold
    if(this->holdFrames && remainFrames) {
        if(remainFrames < this->holdFrames) {
            sample.middleCols(this->holdStart, remainFrames).row(0) *= this->holdLevel;
            return ;
        } else
            sample.middleCols(this->holdStart, this->holdFrames).row(0) *= this->holdLevel;
    }
    remainFrames -= this->holdFrames;

    // Decay
    if(this->decayFrames && remainFrames) {
        if(remainFrames < this->decayFrames)  {
            sample.middleCols(this->decayStart, remainFrames).row(0) *= Eigen::ArrayXf::LinSpaced(remainFrames, this->holdLevel, this->sustainLevel + (this->holdLevel - this->sustainLevel) * (static_cast<float>(remainFrames) / static_cast<float>(this->decayFrames)));
            return ;
        } else
            sample.middleCols(this->decayStart, this->decayFrames).row(0) *= Eigen::ArrayXf::LinSpaced(this->decayFrames, this->holdLevel, this->sustainLevel);
    }
    remainFrames -= this->decayFrames;

    // Sustain
    if(this->sustainFrames && remainFrames) {
        if(remainFrames < this->sustainFrames) {
            sample.middleCols(this->sustainStart, remainFrames) *= sustainLevel;
            return;
        } else
            sample.middleCols(this->sustainStart, this->sustainFrames) *= sustainLevel;
    }
    remainFrames -= this->sustainFrames;

    // Release
    if(this->releaseFrames && remainFrames) {
        if(remainFrames < this->releaseFrames) {
            sample.rightCols(remainFrames).row(0) *= Eigen::ArrayXf::LinSpaced(this->releaseFrames, this->releaseLevel, this->releaseLevel * (static_cast<float>(remainFrames) / static_cast<float>(this->releaseFrames)));
        }
        else
            sample.rightCols(this->releaseFrames).row(0) *= Eigen::ArrayXf::LinSpaced(this->releaseFrames, this->releaseLevel, 0.f);
    }
};

}