#ifndef _WAV_H
#define _WAV_H


#include <cstdio>


namespace prestosynth {

#pragma pack(1)
struct FMTChunk {
	uint16_t format;  // 3: float, 1: short
	uint16_t channelNum;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitPerSample;
};
#pragma pack()

const FMTChunk FMT_FLOAT = {3, 2, 44100, 4, 8, 32};
const FMTChunk FMT_SHORT = {1, 2, 44100, 2, 4, 16};

inline void WAVE_write(const char *filename,
	unsigned short num_channels,
	unsigned long num_samples,
	int sample_rate,
	float *data)
{
	unsigned int bytes_per_sample;
	unsigned int byte_rate;
	unsigned long i;	// counter for samples
	size_t chunkSize = 0;

	bytes_per_sample = 4;
	byte_rate = sample_rate * num_channels * bytes_per_sample;

	FILE *fp = fopen(filename, "wb");
	if (!fp) return;
	//assert(fp);	// make sure it opened

	// write RIFF header
	fwrite("RIFF", 1, 4, fp);
	chunkSize = 36 + bytes_per_sample * num_samples * num_channels;
	fwrite(&chunkSize, 1, 4, fp);
	fwrite("WAVE", 1, 4, fp);

	// write fmt subchunk
	fwrite("fmt ", 1, 4, fp);
	chunkSize = 16;
	fwrite(&chunkSize, 1, 4, fp);
	FMTChunk fmt = FMT_FLOAT;
	fmt.blockAlign = num_channels * bytes_per_sample;
	fmt.sampleRate = sample_rate;
	fwrite(&FMT_FLOAT, sizeof(FMTChunk), 1, fp);

	// write data subchunk
	fwrite("data", 1, 4, fp);
	chunkSize = bytes_per_sample * num_samples * num_channels;
	fwrite(&chunkSize, 1, 4, fp);
	fwrite(data, 4, num_samples*num_channels, fp);

	fclose(fp);
};

}

#endif
