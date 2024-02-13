#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "prestosynth/sequence.h"
#include "prestosynth/synthesizer.h"

int main(int argc, char const *argv[])
{
    psynth::Synthesizer synth(std::string("../example/MuseScore_General.sf3"), 44100, 0, 1);
    psynth::Track track;
    track.preset = 0;
    track.notes =  {
  {1.4654574, 0.21126747, 48, 45},};
    psynth::Sequence sequence;
    sequence.tracks = { track };
    psynth::AudioData audio = synth.render(sequence, true);

    psynth::write_audio("test.wav", audio, 44100);
    return 0;
};
