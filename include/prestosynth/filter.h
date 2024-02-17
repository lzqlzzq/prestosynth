#ifndef _FILTER_H
#define _FILTER_H

#include "util/audio_util.h"
#include "util/convolve.h"

namespace psynth {

class LowPassFilter {
private:
    double ax[3];
    double by[3];
public:
	LowPassFilter(float cutOffFreq, // In Hz
		float sampleRate,  // In Hz
		float resonance);  // In db

	void process(AudioData &sample) const;
};

}

#endif
