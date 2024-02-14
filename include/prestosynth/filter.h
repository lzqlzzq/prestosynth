#ifndef _FILTER_H
#define _FILTER_H

#include "util/audio_util.h"
#include "util/convolve.h"

namespace psynth {

constexpr size_t FILTER_ORDER = 9;

class LowPassFilter {
private:
	psynth::Kernel<FILTER_ORDER + 1> kernel;
public:
	LowPassFilter(float cutOffFreq, // In Hz
		float sampleRate);  // In Hz

	void process(AudioData &sample) const;
};

}

#endif
