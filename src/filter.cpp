#include "prestosynth/filter.h"
#include "prestosynth/util/math_util.h"
#include <cmath>

namespace psynth {

#ifndef M_PI
#define M_PI  3.14159265358979323846264f  // from CRC
#endif
#define SQRT2 1.4142135623730950488

void LowPassFilter::set_params(float filterFc, float filterQ, float sampleRate) {
    if(filterFc >= abscent_to_hz(13499.f))
        return ;

    active = true;

    float q = filterQ - (1.f - 1.f / SQRT2) / (1 + 6 * (filterQ - 1));

    float omega = 2 * M_PI * filterFc / sampleRate;
    float cosVal = std::cos(omega);
    float alpha = std::sin(omega) / (2.f * q);

    float a0 = 1.f + alpha;
    float a1 = -2.f * cosVal;
    float a2 = 1.f - alpha;
    float b1 = 1.f - cosVal;
    float b0 = b1 / 2.f;
    float b2 = b1 / 2.f;

    by[1] = a1 / a0;
    by[2] = a2 / a0;
    ax[0] = b0 / a0;
    ax[1] = b1 / a0;
    ax[2] = b2 / a0;
}

// Following https://github.com/cycfi/q/blob/develop/q_lib/include/q/fx/biquad.hpp#L72
LowPassFilter::LowPassFilter(float filterFc, float filterQ, float sampleRate) {
    set_params(filterFc, filterQ, sampleRate);
};

void LowPassFilter::process(AudioData &sample) const {
    // TODO: Using Eigen expressions to optimize
    if(!active)
        return ;

    float xv[3];
    float yv[3];

    for(int r = 0; r < sample.rows(); r++)
    {
       for(int c = 0; c < sample.cols(); c++)
       {
           xv[2] = xv[1]; xv[1] = xv[0];
           xv[0] = sample(r, c);
           yv[2] = yv[1]; yv[1] = yv[0];

           yv[0] =   (ax[0] * xv[0] + ax[1] * xv[1] + ax[2] * xv[2]
                        - by[1] * yv[0]
                        - by[2] * yv[1]);

           sample(r, c) = yv[0];
       }
    }
};
    

}
