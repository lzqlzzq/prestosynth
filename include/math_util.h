#ifndef _MATH_UTIL_H
#define _MATH_UTIL_H

#include <cmath>

namespace prestosynth {

inline float timecents_to_ms(int16_t timecent) {
    return exp2f(static_cast<float>(timecent) / 1200.0);
};

inline float db_to_amplitude(float db) {
    return exp10(db / 20.0);
};

inline float cb_to_amplitude(float db) {
    return exp10(db / 200.0);
};

}

#endif
