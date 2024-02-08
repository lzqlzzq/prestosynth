#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "sequence.h"
#include "audio_util.h"
#include "math_util.h"
#include "synthesizer.h"

int main(int argc, char const *argv[])
{
    prestosynth::Synthesizer synth(std::string("../example/MuseScore_General.sf3"), 44100, 0);
    prestosynth::Track track;
    track.notes = {{0., 1., 67, 100},
                    {1., .5, 67, 100},
                    {1.5, .5, 70, 100},
                    {2., .5, 72, 100},
                    {2.5, .5, 75, 100},
                    {3., .5, 75, 100},
                    {3.5, .5, 72, 100},
                    {4., 1., 70, 100},
                    {5., .5, 70, 100},
                    {5.5, .5, 72, 100},
                    {6., 1., 70, 100}};
    prestosynth::Sequence sequence;
    sequence.tracks = { track };
    prestosynth::AudioData audio = synth.render_single_thread(sequence, true);

    FILE *f = fopen("l.pcm", "wb");
    fwrite(audio.data(), sizeof(prestosynth::audio_t), audio.shape(1), f);
    fclose(f);

    FILE *f2 = fopen("r.pcm", "wb");
    fwrite(audio.data() + audio.shape(1), sizeof(prestosynth::audio_t), audio.shape(1), f2);
    fclose(f2);

    return 0;
};
