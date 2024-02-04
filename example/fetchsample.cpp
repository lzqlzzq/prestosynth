#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "audio_util.h"
#include "math_util.h"
#include "soundfont.h"

int main(int argc, char const *argv[])
{
    prestosynth::PrestoSoundFont sf(std::string("../example/MuseScore_General.sf3"), 44100, true, 0);
    /*
    prestosynth::AudioData audio = sf.sample(shdr.startOffset,
        shdr.endOffset - shdr.startOffset,
        shdr.sampleRate,
        48000,
        4);
    */
    prestosynth::AudioData audio = sf.build_note(0, 0, 54, 100, 2.);

    FILE *f = fopen("l.pcm", "wb");
    fwrite(audio.data(), sizeof(prestosynth::audio_t), audio.shape(1), f);
    fclose(f);

    FILE *f2 = fopen("r.pcm", "wb");
    fwrite(audio.data() + audio.shape(1), sizeof(prestosynth::audio_t), audio.shape(1), f2);
    fclose(f2);

    return 0;
};
