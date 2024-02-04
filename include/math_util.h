#ifndef _MATH_UTIL_H
#define _MATH_UTIL_H

#include <array>
#include <cmath>
#include "xtensor/xarray.hpp"

namespace prestosynth {

inline float timecents_to_ms(int16_t timecent) {
    return exp2f(static_cast<float>(timecent) / 1200.0) * 1000.0f;
};

inline float timecents_to_s(int16_t timecent) {
    return exp2f(static_cast<float>(timecent) / 1200.0);
};

inline int16_t s_to_timecents(float second) {
    return 1200 * log2f(second);
};

inline float db_to_amplitude(float db) {
    return exp10f(db / 20.0);
};

inline float cb_to_amplitude(int16_t cb) {
    return exp10f(static_cast<float>(cb) / 200.0);
};

inline int16_t amplitude_to_cb(float amp) {
    return 200 * log2f(amp);
};

inline uint32_t s_to_frames(float second, uint16_t sampleRate) {
    return second * sampleRate;
};

namespace math_internal {

inline constexpr float pitch_to_hz(uint8_t pitch) {
    return 440 * exp2f((static_cast<float>(pitch) - 69) / 12);
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

inline float pitch_to_hz(uint8_t pitch) {
    return math_internal::PITCH_HZ_LUT[pitch];
};

}

#endif
