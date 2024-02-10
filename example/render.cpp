#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "sequence.h"
#include "synthesizer.h"

int main(int argc, char const *argv[])
{
    psynth::Synthesizer synth(std::string("../example/MuseScore_General.sf3"), 44100, 0);
    psynth::Track track;
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
    psynth::Sequence sequence;
    sequence.tracks = { track };
    psynth::AudioData audio = synth.render_single_thread(sequence, true);

    psynth::write_audio("test.wav", audio, 44100);
    return 0;
};
