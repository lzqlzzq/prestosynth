#ifndef _SYNTHESIZER_H
#define _SYNTHESIZER_H

#include <map>
#include <vector>
#include <rigtorp/MPMCQueue.h>
#include "prestosynth/sequence.h"
#include "prestosynth/soundfont.h"
#include "prestosynth/util/audio_util.h"

namespace psynth {

struct NoteHead {
    uint32_t duration;
    uint8_t pitch;
    uint8_t velocity;

    bool operator<(const NoteHead &other) const;
};

typedef std::vector<uint32_t> NoteStartPack;

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
typedef rigtorp::mpmc::Queue<AudioData> AudioQueue;

class Synthesizer {
private:
    PrestoSoundFont sf;
    uint32_t sampleRate;
    uint8_t workerNum;

    NoteMap map_notes(const Notes &notes);
    AudioData render_single_thread(const Track &track, bool stereo);
    AudioData render_multi_thread(const Track &track, bool stereo);
    AudioData render_single_thread(const Sequence &sequence, bool stereo);
    AudioData render_multi_thread(const Sequence &sequence, bool stereo);

public:
    Synthesizer(const std::string &sfPath, uint32_t sampleRate, uint8_t quality, uint8_t workerNum = 1);

    AudioData render(const Track &track, bool stereo);
    AudioData render(const Sequence &sequence, bool stereo);
};

}


#endif
