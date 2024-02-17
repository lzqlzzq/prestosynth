#ifndef _FILTER_H
#define _FILTER_H

#include "util/audio_util.h"
#include "util/convolve.h"
#include "soundfont.h"

namespace psynth {

class LowPassFilter {
private:
    double ax[3];
    double by[3];
public:
	LowPassFilter(const SampleAttribute &attr, float sampleRate);

	void process(AudioData &sample) const;
};

}

#endif
