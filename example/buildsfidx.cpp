#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "audiodata.h"
#include "soundfont.h"

int main(int argc, char const *argv[])
{
    prestosynth::PrestoSoundFont sf(std::string("../example/MuseScore_General.sf3"), 44100, true);

    return 0;
};
