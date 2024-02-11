#ifndef _SYNTHESIZER_H
#define _SYNTHESIZER_H

#include <map>
#include <queue>
#include <rigtorp/MPMCQueue.h>
#include "sequence.h"
#include "soundfont.h"
#include "util/audio_util.h"

namespace psynth {

struct NoteHead {
    uint32_t duration;
    uint8_t pitch;
    uint8_t velocity;
};

typedef std::queue<uint32_t> NoteStartPack;

struct PackedNote {
    NoteHead head;
    NoteStartPack startPack;
};

typedef std::map<NoteHead, NoteStartPack> NoteMap;
typedef rigtorp::mpmc::Queue<PackedNote> PackedNoteQueue;

struct NoteAudioPack {
    AudioData audio;
    NoteStartPack startPack;
};

typedef rigtorp::mpmc::Queue<NoteAudioPack> NoteAudioQueue;

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
