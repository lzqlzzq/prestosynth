#ifndef _MATH_UTIL_H
#define _MATH_UTIL_H

#include <array>
#include <cassert>
#include <Eigen/Core>
#include "gcem.hpp"

namespace psynth {

constexpr float timecents_to_ms(int16_t timecent) {
    return gcem::pow(2.f, static_cast<float>(timecent) / 1200.f) * 1000.f;
};

constexpr float timecents_to_s(int16_t timecent) {
    return gcem::pow(2.f, static_cast<float>(timecent) / 1200.f);
};

constexpr int16_t s_to_timecents(float second) {
    return 1200 * gcem::log2(second);
};

constexpr float db_to_amplitude(float db) {
    return gcem::pow(10.f, db / 20.f);
};

constexpr float cb_to_amplitude(float cb) {
    return gcem::pow(10.f, cb / 200.f);
};

constexpr float amplitude_to_cb(float amp) {
    return 200 * gcem::log10(amp);
};

constexpr float amplitude_to_db(float amp) {
    return 20 * gcem::log10(amp);
};

constexpr uint32_t s_to_frames(float second, float sampleRate) {
    return gcem::ceil(second * sampleRate);
};

constexpr float LOG_EPS = db_to_amplitude(-144.f);

namespace math_internal {

constexpr float pitch_to_hz(uint8_t pitch) {
    return 440 * gcem::pow(2.f, (static_cast<float>(pitch) - 69) / 12);
};

constexpr std::array<float, 128> make_pitch_hz_lut() {
    std::array<float, 128> LUT {};

    for (int i = 0; i < 128; ++i) {
        LUT[i] = pitch_to_hz(i);
    }

    return LUT;
};

constexpr std::array<float, 128> PITCH_HZ_LUT = make_pitch_hz_lut();

}

constexpr float pitch_to_hz(uint8_t pitch) {
    return math_internal::PITCH_HZ_LUT[pitch];
};

constexpr float semitone_to_tune(float semitones) {
    return gcem::pow(2.f, semitones / 12.f);
};

constexpr float cent_to_tune(float cent) {
    return gcem::pow(2.f, cent / 1200.f);
};

constexpr float abscent_to_hz(float absCent) {
    return 8.176f * gcem::pow(2.f, absCent / 1200.f);
};

}

#endif
