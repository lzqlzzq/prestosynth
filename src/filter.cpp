#include "prestosynth/filter.h"
#include "prestosynth/util/math_util.h"
#include <cmath>

namespace psynth {

// Following https://github.com/cycfi/q/blob/develop/q_lib/include/q/fx/biquad.hpp#L72
AudioData LowPassFilter::eval_params(const AudioData &freqCurve) const {
	AudioData paramCurve(5, freqCurve.cols()); // (ax0, ax-1, ax-2, by0, by-1)

	AudioData omega = 2 * M_PI * freqCurve / sampleRate;
	AudioData cosVal = omega.cos();
	AudioData alpha = omega.sin() / (2.f * filterQ);

	AudioData a0 = 1.f + alpha;
	AudioData a1 = -2.f * cosVal;
	AudioData a2 = 1.f - alpha;
	AudioData b1 = 1.f - cosVal;
	AudioData b0 = b1 / 2.f;
	AudioData b2 = b1 / 2.f;

	paramCurve.row(2) = b0 / a0;
	paramCurve.row(1) = b1 / a0;
	paramCurve.row(0) = b2 / a0;
	paramCurve.row(4) = -a1 / a0;
	paramCurve.row(3) = -a2 / a0;

	return paramCurve;
};

LowPassFilter::LowPassFilter(float filterQ, float sampleRate) {
	this->filterQ = filterQ - (1.f - 1.f / SQRT2) / (1 + 6 * (filterQ - 1));
	this->sampleRate = sampleRate;
};

void LowPassFilter::process(AudioData &sample, const AudioData &freqCurve) const {
	AudioData buffer(1, 5);
	const AudioData paramCurve = eval_params(freqCurve);

	for(int c = 3; c < sample.cols(); c++)
	{
		buffer.leftCols(2) = buffer.middleCols(1, 2);
		buffer.col(2) = sample.col(c);

		if(freqCurve(0, c) < 13499.f) {
			sample.col(c) = (paramCurve.col(c).transpose() * buffer).sum();
		}

		buffer.col(3) = buffer.col(4);
		buffer.col(4) = sample.col(c);
	}
};

}
