#include "soundfont.h"
#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>


int main(int argc, char const *argv[])
{
    prestosynth::soundfont_internal::SoundFont sf(std::string("../example/MuseScore_General.sf3"));
    std::cout << sf.get_version() << std::endl;
    std::cout << sf.pdta().inst_num() << std::endl;
    std::cout << sf.pdta().inst(11).name << std::endl;
    std::cout << sf.pdta().phdr(0).name << std::endl;

    return 0;
};
