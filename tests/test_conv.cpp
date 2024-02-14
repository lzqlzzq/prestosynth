#include "prestosynth/util/convolve.h"

int main(int argc, char const *argv[]) {
    constexpr size_t kernelSize = 4;
    psynth::Kernel<kernelSize> kernel = Eigen::MatrixXf::Random(kernelSize, 1);
    psynth::AudioData data = Eigen::ArrayXXf::Random(2, 11451419);

    psynth::conv1d(data, kernel);
    psynth::conv1d(data, kernel);
    psynth::conv1d(data, kernel);
    psynth::conv1d(data, kernel);

    return 0;
}
