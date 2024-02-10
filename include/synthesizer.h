#ifndef _SYNTHESIZER_H
#define _SYNTHESIZER_H

#include "sequence.h"
#include "soundfont.h"
#include "audio_util.h"

namespace psynth {

class Synthesizer {
private:
    PrestoSoundFont sf;
    uint32_t sampleRate;

public:
    Synthesizer(const std::string &sfPath, uint32_t sampleRate, uint8_t quality);

    AudioData render_single_thread(const Sequence &sequence, bool stereo);
    AudioData render(const Sequence &sequence, bool stereo, uint8_t workers);
};

}


#endif
