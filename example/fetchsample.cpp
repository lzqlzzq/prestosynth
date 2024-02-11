#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "soundfont.h"

int main(int argc, char const *argv[])
{
    psynth::sf_internal::SoundFont sf(std::string("../example/MuseScore_General.sf2"));

    psynth::AudioData audio = sf.sample(57800641,
        58337736 - 57800641,
        44100,
        44100,
        0);

    psynth::write_audio("test2.wav", audio, 44100);

    return 0;
};
