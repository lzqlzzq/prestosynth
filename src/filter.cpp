#include "prestosynth/filter.h"
#include "prestosynth/util/math_util.h"
#include <cmath>

namespace psynth {

#ifndef M_PI
#define M_PI  3.14159265358979323846264f  // from CRC
#endif
#define SQRT2 1.4142135623730950488

LowPassFilter::LowPassFilter(float cutOffFreq, float sampleRate) {
    double QcRaw  = (M_PI * cutOffFreq) / sampleRate;
    double QcWarp = std::tan(QcRaw);

    double gain = 1 / (1+SQRT2/QcWarp + 2/(QcWarp*QcWarp));

    by[2] = (1 - SQRT2/QcWarp + 2/(QcWarp*QcWarp)) * gain;
    by[1] = (2 - 2 * 2/(QcWarp*QcWarp)) * gain;
    by[0] = 1;

    ax[0] = 1 * gain;
    ax[1] = 2 * gain;
    ax[2] = 1 * gain;
};

void LowPassFilter::process(AudioData &sample) const {
    float xv[3];
    float yv[3];

    for(int r = 0; r < sample.rows(); ++r)
    {
       for(int c = 0; c < sample.cols(); c++)
       {
           xv[2] = xv[1]; xv[1] = xv[0];
           xv[0] = sample(r, c);
           yv[2] = yv[1]; yv[1] = yv[0];

           yv[0] =   (ax[0] * xv[0] + ax[1] * xv[1] + ax[2] * xv[2]
                        - by[1] * yv[0]
                        - by[2] * yv[1]);

           sample.row(r).col(c) = yv[0];
       }
    }
};
    

}
