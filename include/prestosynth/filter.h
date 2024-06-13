#ifndef _FILTER_H
#define _FILTER_H

#include "util/audio_util.h"
#include "soundfont.h"

namespace psynth {

#ifndef M_PI
#define M_PI  3.14159265358979323846264f  // from CRC
#endif
#define SQRT2 1.4142135623730950488

class LowPassFilter {
private:
	float filterQ;
	float sampleRate;

public:
	LowPassFilter(float filterQ, float sampleRate);

	AudioData eval_params(const AudioData &freqCurve) const;
	void process(AudioData &sample, const AudioData &freqCurve) const;
};

}

#endif
