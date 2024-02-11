#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "soundfont.h"

int main(int argc, char const *argv[])
{
    psynth::PrestoSoundFont sf(std::string("../example/MuseScore_General.sf2"), 44100, 0);

    psynth::AudioData audio = sf.build_note(40, 0, 60, 40, 10.f, false);

    psynth::write_audio("test2.wav", audio, 44100);

    return 0;
};
