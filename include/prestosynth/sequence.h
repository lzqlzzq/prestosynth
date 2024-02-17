#ifndef _SEQUENCE_H
#define _SEQUENCE_H

#include <vector>
#include <algorithm>
#include <cstdint>

namespace psynth {

struct Note {
    float start;
    float duration;

    uint8_t pitch;
    uint8_t velocity;

    inline float end() const { return start + duration; };
};

typedef std::vector<Note> Notes;

struct Track {
    uint8_t preset = 0;                   // General MIDI Instrument Number
    uint8_t bank = 0;                     // General MIDI Bank Number
    float volume = 0.f;                   // Track volume in dB
    float pan = 0.f;                      // -0.5 for all left, 0.5 for all right

    Notes notes;

    inline float start() const {
        if(!notes.size()) return 0;
        return std::min_element(notes.begin(), notes.end(), [](const Note &lhs, const Note & rhs) {
            return lhs.start < rhs.start;
        })->start;
    };
    inline float end() const {
        if(!notes.size()) return 0;
        return std::max_element(notes.begin(), notes.end(), [](const Note &lhs, const Note & rhs) {
            return lhs.end() < rhs.end();
        })->end();
    };
    inline size_t note_num() const {
        return notes.size();
    };
};

typedef std::vector<Track> Tracks;

struct Sequence {
    float volume = 0.f;                 // Master volume in dB

    Tracks tracks;

    inline float start() const {
        if(!tracks.size()) return 0;
        return std::min_element(tracks.begin(), tracks.end(), [](const Track &lhs, const Track &rhs) {
            return lhs.start() < rhs.start();
        })->start();
    };
    inline float end() const {
        if(!tracks.size()) return 0;
        return std::max_element(tracks.begin(), tracks.end(), [](const Track &lhs, const Track &rhs) {
            return lhs.end() < rhs.end();
        })->end();
    };
    inline size_t note_num() const {
        size_t n = 0;
        for(const auto &track : tracks) {
            n += track.note_num();
        }
        return n;
    }
};

}

#endif