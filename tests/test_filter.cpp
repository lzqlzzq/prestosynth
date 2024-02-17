#include "prestosynth/filter.h"
#include "prestosynth/util/audio_util.h"
#include "prestosynth/util/math_util.h"

int main(int argc, char const *argv[]) {
    psynth::AudioData noise = Eigen::ArrayXXf::Random(2, 44100 * 10);

    psynth::write_audio("white_noise.wav", noise, 44100);
    psynth::SampleAttribute attr;
    attr.filterFc = 1700.f;
    attr.filterQ = 2.f;

    psynth::LowPassFilter lpf(attr, 44100);

    lpf.process(noise);
    psynth::write_audio("filtered1.wav", noise, 44100);

    return 0;
}
