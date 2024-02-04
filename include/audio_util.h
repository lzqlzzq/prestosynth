#ifndef _AUDIO_UTIL_H
#define _AUDIO_UTIL_H

#include <cstddef>
#include <cstdint>
#include "xtensor/xarray.hpp"
#include "xtensor/xtensor.hpp"
#include "xtensor/xpad.hpp"
#include "xtensor/xview.hpp"

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

AudioData resample_mono(AudioData sampleData, float ratio, uint8_t quality);

}

#endif
