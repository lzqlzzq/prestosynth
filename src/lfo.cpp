#include "prestosynth/lfo.h"
#include "prestosynth/util/math_util.h"

namespace psynth {

LFO::LFO(float delay, float frequency, float sampleRate) {
	delayFrames = s_to_frames(delay, sampleRate);
	halfPeriodFrames = sampleRate / delay / 4 * 2;
};

AudioData LFO::operator()(uint32_t length) const {
	AudioData env(1, length);
	uint32_t curPos = 0;
	if(length < delayFrames) {
		env = 0;
		return env;
	}

	env.leftCols(delayFrames) = 0;
	curPos = delayFrames;

	if(length < halfPeriodFrames / 2) {
		env.middleCols(curPos, length).row(0) = Eigen::ArrayXf::LinSpaced(length, 0.f, length / halfPeriodFrames / 2);
		return env;
	}

	env.middleCols(curPos, halfPeriodFrames / 2).row(0) = Eigen::ArrayXf::LinSpaced(halfPeriodFrames / 2, 0.f, 1.f);
	curPos += halfPeriodFrames / 2;

	bool edge = false;
	while(curPos + halfPeriodFrames <= length) {
		if(edge)
			env.middleCols(curPos, halfPeriodFrames).row(0) = Eigen::ArrayXf::LinSpaced(halfPeriodFrames, -1.f, 1.f);
		else
			env.middleCols(curPos, halfPeriodFrames).row(0) = Eigen::ArrayXf::LinSpaced(halfPeriodFrames, 1.f, -1.f);
		
		edge = !edge;
		curPos += halfPeriodFrames;
	}

	uint32_t remainFrames = length - (curPos + halfPeriodFrames);
	if(edge)
		env.middleCols(curPos, remainFrames).row(0) = Eigen::ArrayXf::LinSpaced(remainFrames, -1.f, static_cast<float>(remainFrames / halfPeriodFrames) * 2.f - 1.f);
	else
		env.middleCols(curPos, remainFrames).row(0) = Eigen::ArrayXf::LinSpaced(remainFrames, 1.f - static_cast<float>(remainFrames / halfPeriodFrames) * 2.f, -1.f);

	return env;
};

}