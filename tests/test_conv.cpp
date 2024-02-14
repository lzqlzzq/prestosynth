#include "prestosynth/util/convolve.h"

int main(int argc, char const *argv[]) {
    psynth::Kernel<2> kernel;
    // psynth::AudioData data = Eigen::ArrayXXf::Random(2, 44100 * 300);
    psynth::AudioData data(1, 10);
    kernel << 1,1;
    data << 1,2,3,4,5,6,7,8,9,10;
    psynth::conv1d(data, kernel);
    std::cout << data << std::endl;

    return 0;
}
