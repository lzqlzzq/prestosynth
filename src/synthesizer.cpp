#include <algorithm>

#include "synthesizer.h"
#include "util/math_util.h"

namespace psynth {

Synthesizer::Synthesizer(const std::string &sfPath, uint32_t sampleRate, uint8_t quality):
    sf(PrestoSoundFont(sfPath, sampleRate, quality)), sampleRate(sampleRate) {};

AudioData Synthesizer::render_single_thread(const Sequence &sequence, bool stereo) {
    AudioData master = Eigen::ArrayXXf::Zero(stereo ? 2 : 1, 1);
    for(const Track &track : sequence.tracks) {
        AudioData trackAudio = Eigen::ArrayXXf::Zero(stereo ? 2 : 1, s_to_frames(track.end(), sampleRate));

        for(const Note &note : track.notes) {
            uint32_t startFrame = s_to_frames(note.start, sampleRate);
            AudioData noteAudio = sf.build_note(track.preset, track.bank, note.pitch, note.velocity, note.duration, stereo);

            if(startFrame + noteAudio.cols() > trackAudio.cols())
                trackAudio.conservativeResize(trackAudio.rows(), startFrame + noteAudio.cols());

            trackAudio.middleCols(startFrame, noteAudio.cols()) += noteAudio;
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
