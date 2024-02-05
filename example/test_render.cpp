#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "audio_util.h"
#include "math_util.h"
#include "synthesizer.h"

int main(int argc, char const *argv[])
{
    prestosynth::Synthesizer synth(std::string("../example/MuseScore_General.sf3"), 44100, 0);

    /*

    prestosynth::AudioData audio = synth(, true);

    FILE *f = fopen("l.pcm", "wb");
    fwrite(audio.data(), sizeof(prestosynth::audio_t), audio.shape(1), f);
    fclose(f);

    FILE *f2 = fopen("r.pcm", "wb");
    fwrite(audio.data() + audio.shape(1), sizeof(prestosynth::audio_t), audio.shape(1), f2);
    fclose(f2);
    */

    return 0;
};
