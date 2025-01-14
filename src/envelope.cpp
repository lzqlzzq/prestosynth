#include "prestosynth/envelope.h"
#include <cassert>
#include <Eigen/Core>

namespace psynth {

Envelope::Envelope(
    const sf_internal::LoopMode loopMode,
    const float delay,
    const float attack,
    const float hold,
    const float decayvol,
    const float sustain,
    const float release,
    const float sampleRate,
    uint32_t durationFrames) {
    using sf_internal::LoopMode;

    if((loopMode == LoopMode::NoLoop | loopMode == LoopMode::Unused) & (sustain != 0.f)) {
        const uint32_t ahFrames = s_to_frames(delay + attack + hold, sampleRate) + 1;
        durationFrames = std::min(durationFrames, ahFrames);
    }

    releaseFrames = s_to_frames(release, sampleRate);
    noteDurationFrames = durationFrames + releaseFrames;

    uint32_t remainFrames = durationFrames;
    uint32_t curPosition = 0;

    // cutoff delay by 0.001s according to the standard
    delayFrames = delay <= 0.001 ? 0 : s_to_frames(delay, sampleRate);
    if(remainFrames <= delayFrames) {
        delayFrames = remainFrames;
        attackStart = curPosition + remainFrames;
        goto release;
    }

    remainFrames -= delayFrames;
    curPosition += delayFrames;

    attackStart = curPosition;
    // cutoff attack by 0.001s according to the standard
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
        // holdLevel = 1.f by default
        holdFrames = remainFrames;
        decayStart = curPosition + remainFrames;

        goto release;
    }

    remainFrames -= holdFrames;
    curPosition += holdFrames;

    decayStart = curPosition;
    decayFrames = delay <= 0.001 ? 0 : s_to_frames(decayvol, sampleRate);
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
    const uint32_t originalReleaseFrames = releaseFrames;

    if(releaseLevel > 0 & releaseLevel < 1.f) {
        const float scaleFactor = -std::log10(-(releaseLevel - 1.f)) / 4.f;
        // Scale factor must be greater than or equal to 0.
        assert(scaleFactor >= 0);
        releaseFrames = static_cast<uint32_t>(std::floor(
            static_cast<float>(releaseFrames) * scaleFactor
        ));
        noteDurationFrames -= originalReleaseFrames - releaseFrames;
    }
};

AudioData Envelope::operator()(const uint32_t length) const {
    AudioData envelope(1, length);
    envelope.setZero();
    uint32_t remainFrames = length;
    // Length must be greater than 0.
    assert(length > 0);

    // Delay
    if(this->delayFrames > 0 & remainFrames > 0) {
        // Decay end position is not equal to attack start position.
        assert(this->decayFrames == this->attackStart);
        if(remainFrames <= this->delayFrames) {
            envelope.leftCols(remainFrames) = 0;
            return envelope;
        } else {
            envelope.leftCols(this->attackStart) = 0;
        }
    }
    remainFrames -= this->delayFrames;

    // Attack
    if(this->attackFrames > 0 & remainFrames > 0) {
        if(remainFrames <= this->attackFrames) {
            envelope.middleCols(this->attackStart, remainFrames).row(0) = Eigen::ArrayXf::LinSpaced(
                remainFrames, 0,
                this->holdLevel * (static_cast<float>(remainFrames) / static_cast<float>(this->attackFrames))
            );
            return envelope;
        } else {
            envelope.middleCols(this->attackStart, this->attackFrames).row(0) = Eigen::ArrayXf::LinSpaced(
                this->attackFrames, 0, this->holdLevel
            );
        }
    }
    remainFrames -= this->attackFrames;

    // Hold
    if(this->holdFrames > 0 & remainFrames > 0) {
        if(remainFrames <= this->holdFrames) {
            envelope.middleCols(this->holdStart, remainFrames).row(0) = this->holdLevel;
            return envelope;
        } else {
            envelope.middleCols(this->holdStart, this->holdFrames).row(0) = this->holdLevel;
        }
    }
    remainFrames -= this->holdFrames;

    // Decay
    // "Hold level is less than sustain level."
    assert(this->holdLevel >= this->sustainLevel);

    if(this->decayFrames > 0 & remainFrames > 0) {
        if(remainFrames <= this->decayFrames)  {
            const auto fillRate = static_cast<float>(remainFrames) / static_cast<float>(this->decayFrames);
            envelope.middleCols(this->decayStart, remainFrames).row(0) = Eigen::ArrayXf::LinSpaced(
                remainFrames, this->holdLevel,
                this->holdLevel - (this->holdLevel - this->sustainLevel) * fillRate
            );
            return envelope;
        } else {
            envelope.middleCols(this->decayStart, this->decayFrames).row(0) = Eigen::ArrayXf::LinSpaced(
                this->decayFrames, this->holdLevel, this->sustainLevel
            );
        }
    }
    remainFrames -= this->decayFrames;

    // Sustain
    if(this->sustainFrames > 0 & remainFrames > 0) {
        if(remainFrames <= this->sustainFrames) {
            envelope.middleCols(this->sustainStart, remainFrames) = sustainLevel;
            return envelope;
        } else {
            envelope.middleCols(this->sustainStart, this->sustainFrames) = sustainLevel;
        }
    }
    remainFrames -= this->sustainFrames;

    // Release
    if(this->releaseFrames > 0 & remainFrames > 0) {
        if(remainFrames <= this->releaseFrames) {
            const float fillRate = static_cast<float>(remainFrames) / static_cast<float>(this->releaseFrames);
            envelope.rightCols(remainFrames).row(0) = Eigen::ArrayXf::LinSpaced(
                this->releaseFrames, this->releaseLevel, this->releaseLevel * (1 - fillRate)
            );
        }
        else {
            envelope.rightCols(this->releaseFrames).row(0) = Eigen::ArrayXf::LinSpaced(
                this->releaseFrames, this->releaseLevel, 0.f
            );
        }
    }
    return envelope;
};

}