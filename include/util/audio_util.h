#ifndef _AUDIO_UTIL_H
#define _AUDIO_UTIL_H

#include <cstddef>
#include <cstdint>

#include "xtensor/xarray.hpp"
#include "xtensor/xtensor.hpp"
#include "xtensor/xview.hpp"
#include "xtensor/xio.hpp"
#include "byte_util.h"
#include "samplerate.h"
#include "wav.h"

namespace psynth {

typedef float audio_t;
typedef xt::xarray<audio_t> AudioData;

inline AudioData zero_audio_data(size_t channels, size_t frames) {
	return xt::zeros<audio_t>({
		channels,
		frames
	});
};

inline AudioData empty_audio_data(size_t channels, size_t frames) {
	return xt::empty<audio_t>({
		channels,
		frames
	});
};

inline AudioData resample_mono(AudioData &sampleData, float ratio, uint8_t quality) {
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

inline void write_audio(const std::string &filePath, AudioData &data, uint16_t sampleRate) {
	xt::xarray<audio_t> transposed = xt::transpose(data, {1, 0});
	WAVE_write(filePath,
		data.shape(0),
		data.shape(1),
		sampleRate,
		const_cast<audio_t*>(transposed.data()));
};

}

#endif
