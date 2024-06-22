#ifndef _MATH_UTIL_H
#define _MATH_UTIL_H

#include <array>
// #include <cassert>
#include <Eigen/Core>
// #include "gcem.hpp"

namespace psynth {

inline float timecents_to_ms(int16_t timecent) {
    return std::pow(2.f, static_cast<float>(timecent) / 1200.f) * 1000.f;
};

inline float timecents_to_s(int16_t timecent) {
    return std::pow(2.f, static_cast<float>(timecent) / 1200.f);
};

inline int16_t s_to_timecents(float second) {
    return 1200 * std::log2(second);
};

inline float db_to_amplitude(float db) {
    return std::pow(10.f, db / 20.f);
};

inline float cb_to_amplitude(float cb) {
    return std::pow(10.f, cb / 200.f);
};

inline float amplitude_to_cb(float amp) {
    return 200 * std::log10(amp);
};

inline float amplitude_to_db(float amp) {
    return 20 * std::log10(amp);
};

inline uint32_t s_to_frames(float second, float sampleRate) {
    return std::ceil(second * sampleRate);
};

inline float LOG_EPS = db_to_amplitude(-144.f);

namespace math_internal {

inline float pitch_to_hz(uint8_t pitch) {
    return 440 * std::pow(2.f, (static_cast<float>(pitch) - 69) / 12);
};

inline std::array<float, 128> make_pitch_hz_lut() {
    std::array<float, 128> LUT {};

    for (int i = 0; i < 128; ++i) {
        LUT[i] = pitch_to_hz(i);
    }

    return LUT;
};

inline std::array<float, 128> PITCH_HZ_LUT = make_pitch_hz_lut();

}

inline float pitch_to_hz(uint8_t pitch) {
    return math_internal::PITCH_HZ_LUT[pitch];
};

inline float semitone_to_tune(float semitones) {
    return std::pow(2.f, semitones / 12.f);
};

inline float cent_to_tune(float cent) {
    return std::pow(2.f, cent / 1200.f);
};

template<typename T>
auto cent_to_tune(T cent) {
    return Eigen::pow(2, cent / 1200.f);
};

inline float abscent_to_hz(float absCent) {
    return 8.176f * std::pow(2.f, absCent / 1200.f);
};

}

#endif
