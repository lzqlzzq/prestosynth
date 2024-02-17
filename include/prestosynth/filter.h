#ifndef _FILTER_H
#define _FILTER_H

#include "util/audio_util.h"
#include "util/convolve.h"
#include "soundfont.h"

namespace psynth {

class LowPassFilter {
private:
	bool active = false;
    float ax[3];
    float by[3];
public:
	void set_params(float filterFc, float filterQ, float sampleRate);
	LowPassFilter(float filterFc, float filterQ, float sampleRate);

	void process(AudioData &sample) const;
};

}

#endif
