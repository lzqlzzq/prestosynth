#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "audiodata.h"
#include "soundfont.h"

int main(int argc, char const *argv[])
{
    prestosynth::soundfont_internal::SoundFont sf(std::string("../example/MuseScore_General.sf2"));
    prestosynth::soundfont_internal::shdrData shdr = sf.shdr(0);
    FILE *f = fopen("a.pcm", "wb");
    prestosynth::AudioData audio = sf.get_sample(shdr, 48000, 4);
    fwrite(audio.data(), sizeof(float), audio.size(), f);
    fclose(f);

    return 0;
};
