#include <algorithm>
#include "synthesizer.h"
#include "math_util.h"

namespace prestosynth {

Synthesizer::Synthesizer(const std::string &sfPath, uint32_t sampleRate, uint8_t quality):
    sf(PrestoSoundFont(sfPath, sampleRate, quality)), sampleRate(sampleRate) {};

AudioData Synthesizer::render_single_thread(const Sequence &sequence, bool stereo) {
    AudioData master = zero_audio_data(stereo ? 2 : 1, 1);
    for(const Track &track : sequence.tracks) {
        AudioData trackAudio = zero_audio_data(stereo ? 2 : 1, s_to_frames(track.end(), sampleRate));

        for(const Note &note : track.notes) {
            uint32_t startFrame = s_to_frames(note.start, sampleRate);
            AudioData noteAudio = sf.build_note(track.preset, track.bank, note.pitch, note.velocity, note.duration, stereo);

            if(startFrame + noteAudio.shape(1) > trackAudio.shape(1))
                trackAudio = xt::pad(trackAudio, {{0, 0}, {startFrame + noteAudio.shape(1) - trackAudio.shape(1)}});

            xt::view(trackAudio, xt::all(), xt::range(startFrame, startFrame + noteAudio.shape(1))) += noteAudio;
        }

        trackAudio *= db_to_amplitude(track.volume);

        if(master.shape(1) < trackAudio.shape(1))
            std::swap(master, trackAudio);

        xt::view(master, xt::all(), xt::range(_, trackAudio.shape(1))) += trackAudio;
    }

    return master * db_to_amplitude(sequence.volume);
};

AudioData Synthesizer::render(const Sequence &sequence, bool stereo, uint8_t workers = 0) {
    if(!workers || workers == 1) return render_single_thread(sequence, stereo);
    // TODO: Implement multithread rendering
};

}