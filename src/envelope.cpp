#include "prestosynth/envelope.h"
#include "prestosynth/util/math_util.h"
#include <cassert>
#include <cmath>
#include <algorithm>

namespace psynth {

// Helper to calculate exponential decay/release rate (multiplier per sample)
// TargetRatio is the desired endpoint relative to the start (e.g., 0.001 for -60dB)
float Envelope::calculateRate(float timeSeconds, float targetRatio, float sampleRate) {
    if (timeSeconds <= 0.0f) {
        // Instantaneous change - rate effectively makes it reach target in one sample
        return targetRatio;
    }
    uint32_t totalSamples = static_cast<uint32_t>(timeSeconds * sampleRate);
    if (totalSamples == 0) {
         return targetRatio; // Avoid division by zero or log(1)
    }
    // rate ^ totalSamples = targetRatio
    // rate = targetRatio ^ (1 / totalSamples)
    // Using exp(log(targetRatio) / totalSamples) is numerically more stable
    return std::exp(std::log(targetRatio) / totalSamples);
}

// Basic timecents to seconds conversion (ignoring key/vel scaling for now)
// static float timecentsToSeconds(float timecents) {
//     if (timecents <= -32768) return 0.0f; // SF2 convention for minimum time
//     return std::pow(2.0f, timecents / 1200.0f);
// }

Envelope::Envelope(float sampleRate) : sampleRate(sampleRate) {
    configure(0, 0, 0, 0, 1.0f, 0); // Default: instant on, full sustain, instant off
}

void Envelope::configure(
    float delaySeconds,
    float attackSeconds,
    float holdSeconds,
    float decaySeconds,
    float sustainLinear, // Already 0-1
    float releaseSeconds,
    int keyNum,
    int velocity
) {
    // TODO: Apply key/velocity scaling to times based on SF2 generators
    // (e.g., keynumToVolEnvHold, velToVolEnvAttack etc.)

    // Clamp times to be non-negative
    delaySeconds = std::max(0.0f, delaySeconds);
    attackSeconds = std::max(0.0f, attackSeconds);
    holdSeconds = std::max(0.0f, holdSeconds);
    decaySeconds = std::max(0.0f, decaySeconds);
    releaseSeconds = std::max(0.0f, releaseSeconds);

    this->delaySamples = static_cast<uint32_t>(delaySeconds * sampleRate);
    this->holdSamples = static_cast<uint32_t>(holdSeconds * sampleRate);

    // Attack: Linear rise to ATTACK_TARGET (slightly > 1.0)
    uint32_t attackSamples = static_cast<uint32_t>(attackSeconds * sampleRate);
    this->attackRate = (attackSamples == 0) ? ATTACK_TARGET : (ATTACK_TARGET / static_cast<float>(attackSamples));

    // Decay: Exponential decay from 1.0 towards sustain level
    // We calculate the rate needed to reach TARGET_RATIO from 1.0 in decaySeconds
    this->decayRate = calculateRate(decaySeconds, TARGET_RATIO, sampleRate);

    // Sustain: Level (0 to 1)
    this->sustainLevel = std::clamp(sustainLinear, 0.0f, 1.0f);

    // Release: Exponential decay from current level towards 0 (TARGET_RATIO)
    this->releaseRate = calculateRate(releaseSeconds, TARGET_RATIO, sampleRate);

    // Initialize state
    start();
}

void Envelope::start() {
    currentStage = Stage::Delay;
    currentValue = 0.0f;
    samplesInStage = 0;
    releaseStartLevel = 0.0f; // Not relevant yet
}

void Envelope::release() {
    if (currentStage != Stage::Done) {
        currentStage = Stage::Release;
        samplesInStage = 0;
        releaseStartLevel = currentValue; // Capture level at release point
    }
}

// Implement generateCurve
AudioData Envelope::generateCurve(uint32_t length) {
    AudioData curve(1, length);
    float* out = curve.data();

    // Reset state for generation
    start(); // Use the start method to reset stage, value, etc.

    for (uint32_t i = 0; i < length; ++i) {
        // Simulate the state machine progression for one sample
        switch (currentStage) {
            case Stage::Delay:
                currentValue = 0.0f;
                samplesInStage++;
                if (samplesInStage >= delaySamples) {
                    currentStage = Stage::Attack;
                    samplesInStage = 0;
                    currentValue = 0.0f; // Start attack from 0
                }
                break;

            case Stage::Attack:
                currentValue += attackRate;
                samplesInStage++;
                if (currentValue >= 1.0f) {
                    currentValue = 1.0f;
                    currentStage = Stage::Hold;
                    samplesInStage = 0;
                }
                break;

            case Stage::Hold:
                currentValue = 1.0f;
                samplesInStage++;
                if (samplesInStage >= holdSamples) {
                    currentStage = Stage::Decay;
                    samplesInStage = 0;
                }
                break;

            case Stage::Decay:
                currentValue *= decayRate;
                samplesInStage++;
                if (currentValue <= sustainLevel) {
                    currentValue = sustainLevel;
                    currentStage = Stage::Sustain;
                    samplesInStage = 0;
                }
                break;

            case Stage::Sustain:
                // If we reach sustain in offline mode, it usually means the note
                // wasn't 'released'. We hold the sustain level indefinitely
                // until the requested length runs out.
                // The 'release' stage is typically triggered externally for offline synths
                // based on note duration, which isn't directly handled here.
                // For simplicity, hold sustain till the end.
                // A more accurate offline model might need a separate release trigger point.
                currentValue = sustainLevel;
                break;

            case Stage::Release:
                // This stage might not be reached naturally in a simple generateCurve
                // unless release() was called *before* generateCurve, which is unusual.
                // Handle it defensively.
                currentValue *= releaseRate;
                if (currentValue <= TARGET_RATIO) {
                    currentValue = 0.0f;
                    currentStage = Stage::Done;
                }
                break;

            case Stage::Done:
                currentValue = 0.0f;
                break;
        }
        out[i] = currentValue;
    }
    // Note: The 'release' part of the ADSR is implicitly handled by the total 'length'
    // requested, or needs an external trigger mechanism in an offline context.
    // This function generates the curve up to 'length' based on Attack, Decay, Sustain.
    return curve;
}

}