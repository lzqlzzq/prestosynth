#ifndef ENVELOPE_H
#define ENVELOPE_H

#include "prestosynth/soundfont_internal.h"
#include "prestosynth/util/audio_util.h"
#include <cmath>
#include <cstdint>

namespace psynth {

// Forward declaration if necessary
// class Synthesizer; // Assuming sample rate comes from a synth context

class Envelope {
public:
    enum class Stage {
        Delay,
        Attack,
        Hold,
        Decay,
        Sustain,
        Release,
        Done
    };

private:
    // Parameters (converted from timecents/seconds to rates/levels)
    float sampleRate;
    float attackRate;   // Rate of increase per sample (1.0 / attack_samples)
    float decayRate;    // Multiplier per sample for exponential decay
    float releaseRate;  // Multiplier per sample for exponential release
    float sustainLevel; // Level (0.0 to 1.0)
    uint32_t delaySamples;
    uint32_t holdSamples;

    // State
    Stage currentStage = Stage::Delay;
    float currentValue = 0.0f;
    uint32_t samplesInStage = 0;
    float releaseStartLevel = 0.0f; // Level when release phase begins

    // Constants for exponential decay calculation (similar to meltysynth)
    static constexpr float TARGET_RATIO = 0.001f; // Target level for decay/release (close to zero)
    static constexpr float ATTACK_TARGET = 1.0f + TARGET_RATIO; // Slightly above 1 for attack overshoot avoidance
    static constexpr float DECAY_TARGET = TARGET_RATIO; // Target for decay

    // Helper to convert timecents to samples
    static uint32_t timecentsToSamples(float timecents, float sampleRate);
    // Helper to calculate exponential rate
    static float calculateRate(float timeSeconds, float targetRatio, float sampleRate);

public:
    Envelope(float sampleRate);

    // Configure the envelope based on SoundFont parameters (times in seconds)
    void configure(
        float delaySeconds,   // volEnvDelay
        float attackSeconds,  // volEnvAttack
        float holdSeconds,    // volEnvHold
        float decaySeconds,   // volEnvDecay
        float sustainLevel,   // volEnvSustain (0-1 linear, from 0-1000 tenths of %)
        float releaseSeconds, // volEnvRelease
        int keyNum = 60,      // For potential key->time scaling (not implemented yet)
        int velocity = 100    // For potential velocity->time scaling (not implemented yet)
    );

    // Trigger the start of the envelope (when noteOn occurs)
    void start();

    // Trigger the release phase (when noteOff occurs)
    void release();

    // Process the envelope for a block of samples,
    // writing the envelope value into the output buffer.
    void process(AudioData& outputBuffer, uint32_t numSamples);

    // Get the current envelope value
    float getValue() const { return currentValue; }

    // Check if the envelope has finished (Stage::Done)
    bool isFinished() const { return currentStage == Stage::Done; }

    // Get the current stage
    Stage getStage() const { return currentStage; }

    // Generate the envelope curve for a given length
    AudioData generateCurve(uint32_t length);
};

}

#endif //ENVELOPE_H
