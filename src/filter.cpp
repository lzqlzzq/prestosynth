#include "prestosynth/filter.h"
#include "prestosynth/util/math_util.h"
#include <cmath>

namespace psynth {

#ifndef M_PI
#define M_PI  3.14159265358979323846264f  // from CRC
#endif

LowPassFilter::LowPassFilter(float cutOffFreq, float sampleRate, float Q) {
    double fc = 2 * M_PI * cutOffFreq / sampleRate;

    // Kaiser window
    Eigen::VectorXf w(FILTER_ORDER);
    for (int n = 0; n < FILTER_ORDER; ++n) {
        double arg = Q * std::sqrt(1 - std::pow((2 * n) / (FILTER_ORDER - 1.0) - 1, 2));
        w(n) = std::real(std::cyl_bessel_i(0, arg)) / std::real(std::cyl_bessel_i(0, Q));
    }

    // Build FIR kernel
    for(int n = 0; n < FILTER_ORDER; n++) {
        if(n == FILTER_ORDER / 2)
            kernel(n) = fc / M_PI;
        else
            kernel(n) = std::sin(fc * (n - FILTER_ORDER/2)) / (M_PI * (n - FILTER_ORDER/2));
        kernel(n) *= Q;
    }

    // Normalize
    kernel /= kernel.sum();
};

void LowPassFilter::process(AudioData &sample) const {
    conv1d(sample, kernel);
};
    

}
