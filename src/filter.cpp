#include "prestosynth/filter.h"
#include "prestosynth/util/math_util.h"
#include <cmath>

namespace psynth {

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
    std::cout << wc << std::endl;
    std::cout << kernel << std::endl;
};

void LowPassFilter::process(AudioData &sample) const {
    conv1d(sample, kernel);
    conv1d(sample, kernel);
    conv1d(sample, kernel);
    conv1d(sample, kernel);
};
    

}
