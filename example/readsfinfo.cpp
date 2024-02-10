#include "soundfont.h"
#include <string>
#include <iostream>
#include <cstddef>
#include <cstdint>


int main(int argc, char const *argv[])
{
    psynth::sf_internal::SoundFont sf(std::string("../example/MuseScore_General.sf2"));

    std::cout << sf.inst(142).bagIdx << std::endl;
    std::cout << sf.inst(143).bagIdx << std::endl;
    std::cout << sf.inst(198).name << std::endl;
    std::cout << sf.inst(198).bagIdx << std::endl;
    std::cout << sf.inst(199).bagIdx << std::endl;
    std::cout << sf.ibag(2452).genIdx << std::endl;
    std::cout << sf.ibag(2453).genIdx << std::endl;
    std::cout << sf.shdr(1100).name << std::endl;

    for (int i = 11810; i < 11819; ++i)
    {
        std::cout << uint16_t(sf.igen(i).genOper) << std::endl;
        std::cout << sf.igen(i).amount.sAmount << std::endl;
    }

    return 0;
};
