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

	auto a0 = 1.f + alpha;
	auto a1 = -2.f * cosVal;
	auto a2 = 1.f - alpha;
	auto b1 = 1.f - cosVal;
	auto b0 = b1 / 2.f;
	auto b2 = b1 / 2.f;

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
	// AudioData buffer(1, 5);
	float buffer[5] = {0, 0, 0, 0, 0};
	// const AudioData paramCurve = eval_params(freqCurve);
	const auto omega_weight = 2 * M_PI / sampleRate;
	const auto alpha_weight = 1 / (2.f * filterQ);
	auto * data = sample.data();

	auto eval_sample = [omega_weight, alpha_weight, buffer](const float freq) {
		const auto omega = omega_weight * freq;
		const auto cosVal = std::cos(omega);
		const auto alpha = alpha_weight * std::sin(omega);
		const auto a0 = 1.f + alpha;
		const auto a1 = -2.f * cosVal;
		const auto a2 = 1.f - alpha;
		const auto b1 = 1.f - cosVal;
		const auto b0 = b1 / 2.f;
		const auto b2 = b0;

		auto sum = b2 * buffer[0];
		sum += b1 * buffer[1];
		sum += b0 * buffer[2];
		sum += -a2 * buffer[3];
		sum += -a1  * buffer[4];
		return sum / a0;
	};

	for(int c = 3; c < sample.cols(); c++) {
		float *cur = data + c;
		buffer[0] = buffer[1];
		buffer[1] = buffer[2];
		buffer[2] = *cur;
		if(const float freq = *(freqCurve.data() + c); freq < 13499.f) {
			*cur = eval_sample(freq);
		}
		buffer[3] = buffer[4];
		buffer[4] = *cur;
	}
};

}
