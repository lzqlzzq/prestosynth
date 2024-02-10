#ifndef _WAV_H
#define _WAV_H

#include <cstdio>
#include <string>


namespace psynth {

struct FMTChunk {
	uint16_t format;  // 3: float, 1: short
	uint16_t channelNum;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitPerSample;
};
static_assert(sizeof(FMTChunk) == 16);

const FMTChunk FMT_FLOAT = {3, 2, 44100, 4, 8, 32};
const FMTChunk FMT_SHORT = {1, 2, 44100, 2, 4, 16};

inline void WAVE_write(const std::string &filename,
	unsigned short num_channels,
	unsigned long num_samples,
	int sample_rate,
	float *data) {
	size_t chunkSize = 0;

	FILE *fp = fopen(const_cast<char*>(filename.c_str()), "wb");
	if (!fp) throw std::ios_base::failure("Cannot write the wav file " + filename + "!");

	// write RIFF header
	fwrite("RIFF", 1, 4, fp);
	chunkSize = 36 + sizeof(float) * num_samples * num_channels;
	fwrite(&chunkSize, 1, 4, fp);
	fwrite("WAVE", 1, 4, fp);

	// write fmt subchunk
	fwrite("fmt ", 1, 4, fp);
	chunkSize = 16;
	fwrite(&chunkSize, 1, 4, fp);
	FMTChunk fmt = FMT_FLOAT;
	fmt.channelNum = num_channels;
	fmt.blockAlign = num_channels * sizeof(float);
	fmt.sampleRate = sample_rate;
	fmt.byteRate = sample_rate * num_channels * sizeof(float);
	fwrite(&FMT_FLOAT, sizeof(FMTChunk), 1, fp);

	// write data subchunk
	fwrite("data", 1, 4, fp);
	chunkSize = sizeof(float) * num_samples * num_channels;
	fwrite(&chunkSize, 1, 4, fp);
	fwrite(data, sizeof(float), num_samples * num_channels, fp);

	fclose(fp);
};

}

#endif
