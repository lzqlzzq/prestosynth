#ifndef _LFO_H
#define _LFO_H

#include "util/audio_util.h"

namespace psynth {

class LFO {
public:
	uint32_t delayFrames;
	uint32_t halfPeriodFrames;

	LFO(float delay, float frequency, float sampleRate);

	AudioData operator()(uint32_t length) const;
};

}


#endif
