#ifndef _FILTER_H
#define _FILTER_H

#include "util/audio_util.h"
#include "soundfont.h"

namespace psynth {

#ifndef M_PI
#define M_PI  3.14159265358979323846264f  // from CRC
#endif
#define SQRT2 1.4142135623730950488

class LowPassFilter {
private:
	float sampleRate;
	float q; // Biquad Q parameter, derived from SF2 resonance

	// State variables (Transposed Direct Form II)
	float z1 = 0.f;
	float z2 = 0.f;

	// Coefficients (calculated per sample based on freqCurve)
	// Store them temporarily if needed, but calculation might happen inline
	// float b0, b1, b2, a1, a2; // Example coefficients

	// Pre-calculated constant for Q conversion from resonance peak height
	static const float resonancePeakOffset;

	// Helper to calculate coefficients
	void calculate_coeffs(float normalizedFreq, float& b0, float& b1, float& b2, float& a1, float& a2) const;


public:
	// Constructor takes resonance peak height (linear gain, >= 1) and sample rate
	LowPassFilter(float resonanceDb, float sampleRate);

	// Reset filter state
	void reset();

	// Process an audio block with a corresponding frequency curve (in Hz)
	// Modifies the sample block in-place.
	void process(AudioData &sample, const AudioData &freqCurveHz);
};

}

#endif
