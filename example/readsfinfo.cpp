#include "soundfont.h"
#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>


int main(int argc, char const *argv[])
{
    prestosynth::soundfont_internal::SoundFont sf(std::string("../example/MuseScore_General.sf2"));
    std::cout << sf.version() << std::endl;
    std::cout << sf.inst_num() << std::endl;
    std::cout << sf.inst(11).name << std::endl;
    std::cout << sf.shdr(0).name << std::endl;
    std::cout << sf.shdr(0).sampleRate << std::endl;
    std::cout << sf.shdr(0).startOffset << std::endl;
    std::cout << sf.shdr(0).endOffset << std::endl;
    std::cout << sf.shdr(0).startLoop << std::endl;
    std::cout << sf.shdr(0).endLoop << std::endl;

    return 0;
};
