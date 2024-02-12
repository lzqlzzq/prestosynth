#include <algorithm>

#include "prestosynth/synthesizer.h"
#include "prestosynth/util/math_util.h"

namespace psynth {

bool NoteHead::operator<(const NoteHead &other) const {
    return (this->duration < other.duration) ||
            (this->duration == other.duration && this->pitch < other.pitch) ||
            (this->duration == other.duration && this->pitch < other.pitch && this->velocity < other.velocity);
};

Synthesizer::Synthesizer(const std::string &sfPath, uint32_t sampleRate, uint8_t quality):
    sf(PrestoSoundFont(sfPath, sampleRate, quality)), sampleRate(sampleRate) {};

NoteMap Synthesizer::map_notes(const Notes &notes) {
    NoteMap noteMap;
    for(const Note &note : notes) {
        uint32_t startFrame = s_to_frames(note.start, sampleRate);
        uint32_t durationFrame = s_to_frames(note.duration, sampleRate);

        NoteHead head = {durationFrame, note.pitch, note.velocity};

        if(!noteMap.count(head))
            noteMap[head] = NoteStartPack();

        noteMap[head].push(startFrame);
    }
    return noteMap;
};

AudioData Synthesizer::render_single_thread(const Sequence &sequence, bool stereo) {
    AudioData master = Eigen::ArrayXXf::Zero(stereo ? 2 : 1, 1);
    for(const Track &track : sequence.tracks) {
        AudioData trackAudio = Eigen::ArrayXXf::Zero(stereo ? 2 : 1, s_to_frames(track.end(), sampleRate));

        for(const auto &pack : map_notes(track.notes)) {
            const auto &head = pack.first;
            auto startFrames = pack.second;
            AudioData noteAudio = sf.build_note(
                track.preset,
                track.bank,
                head.pitch,
                head.velocity,
                head.duration,
                stereo);

            while(!startFrames.empty()) {
                uint32_t startFrame = startFrames.front();
                startFrames.pop();

                if(startFrame + noteAudio.cols() > trackAudio.cols())
                    trackAudio.conservativeResize(Eigen::NoChange, startFrame + noteAudio.cols());

                trackAudio.middleCols(startFrame, noteAudio.cols()) += noteAudio;
            }
        }

        trackAudio *= db_to_amplitude(track.volume);

        if(master.cols() < trackAudio.cols())
            std::swap(master, trackAudio);

        master.leftCols(trackAudio.cols()) += trackAudio;
    }

    return master * db_to_amplitude(sequence.volume);
};

AudioData Synthesizer::render(const Sequence &sequence, bool stereo, uint8_t workers = 0) {
    if(!workers || workers == 1) return render_single_thread(sequence, stereo);
    // TODO: Implement multithread rendering
    // Dummy multithreading for now.
    return render_single_thread(sequence, stereo);
};

}
