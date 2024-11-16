#include "prestosynth/lfo.h"
#include "prestosynth/util/math_util.h"
#include <cassert>

namespace psynth {

LFO::LFO(const float delay, const float frequency, const float sampleRate) {
	delayFrames = s_to_frames(delay, sampleRate);
	halfPeriodFrames = static_cast<uint32_t>(std::floor(
		sampleRate / frequency / 4 * 2
	));
};

AudioData LFO::operator()(const uint32_t length) const {
	AudioData env(1, length);

	// 1. Delay
	//    env[:delayFrames] = 0
	//    so we can just return if length <= delayFrames
	if(length <= delayFrames) {
		env = 0;
		return env;
	}

	uint32_t curPos = 0;
	env.leftCols(delayFrames) = 0;
	curPos = delayFrames;

	// 2. Half Period Frames
	//	  env[delayFrames: delayFrames + quarterPeriodFrames] = np.linspace(0, 1, quarterPeriodFrames)
	//	  so we can just return if length <= halfPeriodFrames // 2 + delayFrames
	//    env[delayFrames: length] = np.linspace(0, (length - delayFrames) / quarterPeriodFrames, length - delayFrames)

	const uint32_t quarterPeriodFrames = halfPeriodFrames / 2; // floor division

	if(length <= curPos + quarterPeriodFrames) {
		const auto numCols = length - curPos;
		env.middleCols(curPos, numCols).row(0) = Eigen::ArrayXf::LinSpaced(
			numCols, 0.f,
			static_cast<float>(numCols) / static_cast<float>(quarterPeriodFrames)
		);
		return env;
	}

	env.middleCols(curPos, quarterPeriodFrames).row(0) = Eigen::ArrayXf::LinSpaced(
		quarterPeriodFrames, 0.f, 1.f
	);

	curPos += quarterPeriodFrames;

	bool risingEdge = false;
	while(curPos + halfPeriodFrames <= length) {
		if(risingEdge) {
			env.middleCols(curPos, halfPeriodFrames).row(0) = Eigen::ArrayXf::LinSpaced(
				halfPeriodFrames, -1.f, 1.f
			);
		} else {
			env.middleCols(curPos, halfPeriodFrames).row(0) = Eigen::ArrayXf::LinSpaced(
				halfPeriodFrames, 1.f, -1.f
			);
		}

		risingEdge = !risingEdge;
		curPos += halfPeriodFrames;
	}

	// 3. Remainder
	//    return if nothing left

	if (curPos == length) {
		return env;
	}

	// curPos should be less than length at this point
	assert(curPos < length);

	const uint32_t remainFrames = length - curPos;
	if(risingEdge) {
		env.rightCols(remainFrames).row(0) = Eigen::ArrayXf::LinSpaced(
			remainFrames, -1.f,
			static_cast<float>(remainFrames) / static_cast<float>(quarterPeriodFrames) - 1.f
		);
	} else {
		env.rightCols(remainFrames).row(0) = Eigen::ArrayXf::LinSpaced(
			remainFrames, 1.f,
			1.f - static_cast<float>(remainFrames) / static_cast<float>(quarterPeriodFrames)
		);
	}

	return env;
};

}