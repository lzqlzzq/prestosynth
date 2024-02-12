#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "prestosynth/soundfont_internal.h"
#include "prestosynth/util/audio_util.h"

int main(int argc, char const *argv[])
{
    psynth::sf_internal::SoundFont sf(std::string("../example/MuseScore_General.sf2"));

    psynth::AudioData audio = sf.sample(23408524,
        23451868 - 23408524,
        44100,
        44100,
        0);

    psynth::write_audio("test2.wav", audio, 44100);

    return 0;
};
