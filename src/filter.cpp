#include "prestosynth/filter.h"
#include "prestosynth/util/math_util.h"
#include <cmath>

namespace psynth {

#ifndef M_PI
#define M_PI  3.14159265358979323846264f  // from CRC
#endif

LowPassFilter::LowPassFilter(float cutOffFreq, float sampleRate) {
    // Butterworth
    double wc = 2 * cutOffFreq / sampleRate;

    for(int i = 0; i < FILTER_ORDER; ++i)
    {
        int idx = FILTER_ORDER - i;
        double sincArg = wc * i;

        if(i)
            kernel(idx) = sin(M_PI * sincArg) / (M_PI * sincArg);
        else
            kernel(idx) = 1.;

        // Window function
        kernel(idx) *= pow(cos(wc * (i + 0.5)) / cos(wc/2), 4);
    }

    kernel /= kernel.sum();
};

void LowPassFilter::process(AudioData &sample) const {
    conv1d(sample, kernel);
    conv1d(sample, kernel);
    conv1d(sample, kernel);
    conv1d(sample, kernel);
};
    

}
