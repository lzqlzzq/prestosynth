#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "soundfont.h"

int main(int argc, char const *argv[])
{
    psynth::PrestoSoundFont sf(std::string("../example/MuseScore_General.sf2"), 44100, 0);
    /*
    prestosynth::AudioData audio = sf.sample(shdr.startOffset,
        shdr.endOffset - shdr.startOffset,
        shdr.sampleRate,
        48000,
        4);
    */
    psynth::AudioData audio = sf.build_note(0, 0, 52, 100, 2., true);

    FILE *f = fopen("l.pcm", "wb");
    fwrite(audio.data(), sizeof(psynth::audio_t), audio.shape(1), f);
    fclose(f);

    FILE *f2 = fopen("r.pcm", "wb");
    fwrite(audio.data() + audio.shape(1), sizeof(psynth::audio_t), audio.shape(1), f2);
    fclose(f2);

    return 0;
};
