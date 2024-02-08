#ifndef _AUDIO_UTIL_H
#define _AUDIO_UTIL_H

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "xtensor/xarray.hpp"
#include "xtensor/xtensor.hpp"
#include "xtensor/xpad.hpp"
#include "xtensor/xview.hpp"
#include "byte_util.h"
#include "wav.h"

namespace prestosynth {

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

AudioData resample_mono(AudioData &sampleData, float ratio, uint8_t quality);

inline void write_audio(const std::string &filePath, AudioData &data, uint16_t sampleRate) {
	xt::xarray<audio_t> transposed = xt::transpose(data, {1, 0});
	WAVE_write(const_cast<char*>(filePath.c_str()),
		data.shape(0) * data.shape(1),
		const_cast<audio_t*>(transposed.data()),
		sampleRate,
		data.shape(0));
};

}

#endif
