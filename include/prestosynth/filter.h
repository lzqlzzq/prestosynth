#ifndef _FILTER_H
#define _FILTER_H

#include "util/audio_util.h"
#include "util/convolve.h"

namespace psynth {

constexpr size_t FILTER_ORDER = 5;

class LowPassFilter {
private:
	psynth::Kernel<FILTER_ORDER> kernel;
public:
	LowPassFilter(float cutOffFreq, // In Hz
		float sampleRate,  // In Hz
		float Q);  // In Db

	void process(AudioData &sample) const;
};

}

#endif
