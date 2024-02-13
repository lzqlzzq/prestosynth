#ifndef _AUDIO_UTIL_H
#define _AUDIO_UTIL_H

#include <cstddef>
#include <cstdint>
#include <Eigen/Core>
#include "prestosynth/util/byte_util.h"
#include "samplerate.h"
#include "prestosynth/wav.h"

namespace psynth {

typedef float audio_t;
typedef Eigen::ArrayXXf AudioData;

inline AudioData resample_mono(AudioData &sampleData, float ratio, uint8_t quality) {
	SRC_DATA srcInfo;
	srcInfo.src_ratio = ratio;
	srcInfo.input_frames = sampleData.cols();
	srcInfo.output_frames = sampleData.cols() * srcInfo.src_ratio;

	AudioData resampledData = Eigen::ArrayXXf::Zero(1, srcInfo.output_frames);

	srcInfo.data_in = sampleData.data();
	srcInfo.data_out = resampledData.data();

	src_simple(&srcInfo, 4 - quality, 1);

	return resampledData;
};

inline void write_audio(const std::string &filePath, const AudioData &data, uint16_t sampleRate, bool writeS16 = false) {
	WAVE_write(filePath,
		data.rows(),
		data.cols(),
		sampleRate,
		const_cast<audio_t*>(data.transpose().data()),
		writeS16);
};

}

#endif
