#ifndef _FILTER_H
#define _FILTER_H

#include "util/audio_util.h"
#include "util/convolve.h"

namespace psynth {

constexpr size_t FILTER_ORDER = 7;

class LowPassFilter {
private:
	psynth::Kernel<FILTER_ORDER + 1> kernel;
    double ax[3];
    double by[3];
public:
	LowPassFilter(float cutOffFreq, // In Hz
		float sampleRate);  // In Hz

	void process(AudioData &sample) const;
};

}

#endif
