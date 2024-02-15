#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "prestosynth/soundfont.h"

int main(int argc, char const *argv[])
{
    psynth::PrestoSoundFont sf(std::string("../example/MuseScore_General.sf3"), 44100, 0);

    psynth::AudioData audio = sf.build_note(45, 0, 60, 127, 10 * 44100, true);

    psynth::write_audio("test3.wav", audio, 44100);

    return 0;
};
