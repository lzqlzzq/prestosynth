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
    track.preset = 40;
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
                    {6., 2., 70, 100},
                    {8., 1., 67, 100},
                    {9., .5, 67, 100},
                    {9.5, .5, 70, 100},
                    {10., .5, 72, 100},
                    {10.5, .5, 75, 100},
                    {11., .5, 75, 100},
                    {11.5, .5, 72, 100},
                    {12., 1., 70, 100},
                    {13., .5, 70, 100},
                    {13.5, .5, 72, 100},
                    {14., 2., 70, 100},
                    {16., 1., 70, 100},
                    {17., 1., 70, 100},
                    {18., 1., 70, 100},
                    {19., .5, 67, 100},
                    {19.5, .5, 70, 100},
                    {20., 1., 72, 100},
                    {21., 1., 72, 100},
                    {22., 2., 70, 100},
                    {24., 1., 67, 100},
                    {25., .5, 65, 100},
                    {25.5, .5, 67, 100},
                    {26., 1., 70, 100},
                    {27., .5, 67, 100},
                    {27.5, .5, 65, 100},
                    {28., 1., 63, 100},
                    {29., .5, 63, 100},
                    {29.5, .5, 65, 100},
                    {30., 2., 63, 100}};
    psynth::Sequence sequence;
    sequence.tracks = { track };
    psynth::AudioData audio = synth.render_single_thread(sequence, true);

    psynth::write_audio("test.wav", audio, 44100);
    return 0;
};
