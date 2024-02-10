#include "audio_util.h"
#include "samplerate.h"
#include "xtensor/xio.hpp"
#include "wav.h"

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

void write_audio(const std::string &filePath, AudioData &data, uint16_t sampleRate) {
    xt::xarray<audio_t> transposed = xt::transpose(data, {1, 0});
    WAVE_write(const_cast<char*>(filePath.c_str()),
        data.shape(0),
        data.shape(1),
        sampleRate,
        const_cast<audio_t*>(transposed.data()));
};

}
