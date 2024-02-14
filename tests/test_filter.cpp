#include "prestosynth/filter.h"
#include "prestosynth/util/audio_util.h"

int main(int argc, char const *argv[]) {
    psynth::AudioData noise = Eigen::ArrayXXf::Random(2, 44100 * 10);

    psynth::write_audio("white_noise.wav", noise, 44100);

    psynth::LowPassFilter lpf(8000, 44100, .5f);

    lpf.process(noise);
    psynth::write_audio("filtered.wav", noise, 44100);

    return 0;
}
