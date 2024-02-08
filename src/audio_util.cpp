#include "audio_util.h"
#include "samplerate.h"
#include "xtensor/xio.hpp"

namespace prestosynth {

AudioData resample_mono(AudioData &sampleData, float ratio, uint8_t quality) {
    SRC_DATA srcInfo;
    srcInfo.src_ratio = ratio;
    srcInfo.input_frames = sampleData.shape(1);
    srcInfo.output_frames = sampleData.shape(1) * srcInfo.src_ratio;

    AudioData resampledData = zero_audio_data(1, srcInfo.output_frames);

    srcInfo.data_in = sampleData.data();
    srcInfo.data_out = resampledData.data();

    src_simple(&srcInfo, 4 - quality, 1);

    return resampledData;
};

}
