#include "prestosynth/filter.h"
#include "prestosynth/util/math_util.h"
#include <cmath>
#include <cassert>

namespace psynth {

// Constant from meltysynth, used for resonance to Q conversion
const float LowPassFilter::resonancePeakOffset = 1.0f - 1.0f / static_cast<float>(SQRT2);

// Calculate filter coefficients based on normalized frequency (freq/sampleRate) and Q
// Using formulas similar to meltysynth/Audio Biquad Cookbook
void LowPassFilter::calculate_coeffs(float normalizedFreq, float& b0, float& b1, float& b2, float& a1, float& a2) const {
	const float w = 2.f * M_PI * normalizedFreq;
	const float cosw = std::cos(w);
	const float alpha = std::sin(w) / (2.f * this->q);

	const float coeff_a0 = 1.f + alpha; // Note: This is the denominator 'a0' from cookbook formulas

	b0 = (1.f - cosw) / 2.f;
	b1 = 1.f - cosw;
	b2 = b0;
	// a0 = 1 + alpha; // Implicit in normalization
	a1 = -2.f * cosw;
	a2 = 1.f - alpha;

	// Normalize coefficients by a0
	b0 /= coeff_a0;
	b1 /= coeff_a0;
	b2 /= coeff_a0;
	a1 /= coeff_a0; // Note: Cookbook a1/a2 have opposite sign convention
	a2 /= coeff_a0; // Note: Cookbook a1/a2 have opposite sign convention
}

LowPassFilter::LowPassFilter(float resonanceDb, float sampleRate) {
	// Convert SF2 resonance (dB peak height) to linear Q factor
	// Clamp resonanceDb to avoid issues (e.g., >= 0 dB)
	resonanceDb = std::max(0.f, resonanceDb);
	float resonance_linear = std::pow(10.f, resonanceDb / 20.f);

	// Formula from meltysynth to approximate Q for desired peak gain
	// Clamp resonance_linear >= 1 (0dB) for the formula
	resonance_linear = std::max(1.f, resonance_linear);
	this->q = resonance_linear - resonancePeakOffset / (1.f + 6.f * (resonance_linear - 1.f));
	// Ensure q is positive
	this->q = std::max(0.001f, this->q); // Avoid q=0

	this->sampleRate = sampleRate;
	reset();
}

void LowPassFilter::reset() {
	z1 = 0.f;
	z2 = 0.f;
}

void LowPassFilter::process(AudioData &sample, const AudioData &freqCurveHz) {
	assert(sample.cols() == freqCurveHz.cols());

	float b0, b1, b2, a1, a2; // Coefficients

	// Using Transposed Direct Form II structure:
	// y[n] = b0*x[n] + z1[n-1]
	// z1[n] = b1*x[n] + z2[n-1] - a1*y[n]
	// z2[n] = b2*x[n] - a2*y[n]

	// SF2 spec: Filter cutoff is in absolute cents.
	// We expect freqCurveHz to be already converted to Hz.

	float* sampleData = sample.data();
	const float* freqData = freqCurveHz.data();

	for (long i = 0; i < sample.cols(); ++i) {
		float currentFreq = freqData[i];
		float inputSample = sampleData[i];

		// Optimization: Bypass filter if cutoff is too high (e.g., near Nyquist)
		// or too low (effectively DC block, though this is LPF)
		// Using 19kHz like FluidSynth as a threshold
		constexpr float freqThresholdHigh = 19000.0f;
		constexpr float freqThresholdLow = 10.0f; // Avoid calculation issues at near-zero Hz

		if (currentFreq >= freqThresholdHigh) {
			// Filter bypassed, state variables should represent passthrough
			z1 = b1 * inputSample - a1 * inputSample + z2;
			z2 = b2 * inputSample - a2 * inputSample;
			// Output is just inputSample, already in place
		} else if (currentFreq <= freqThresholdLow) {
			// Effectively DC blocking or very low cutoff - treat as 0 output?
			// Or calculate coeffs? For now, let's just zero state and output.
			// This might cause clicks if freq jumps rapidly.
			// A smoother approach might be needed.
			z1 = 0.f;
			z2 = 0.f;
			sampleData[i] = 0.f;
		} else {
			// Calculate normalized frequency and coefficients
			float normalizedFreq = currentFreq / sampleRate;
			calculate_coeffs(normalizedFreq, b0, b1, b2, a1, a2);

			// Apply filter (Transposed Direct Form II)
			float outputSample = b0 * inputSample + z1;
			z1 = b1 * inputSample - a1 * outputSample + z2;
			z2 = b2 * inputSample - a2 * outputSample;

			sampleData[i] = outputSample;
		}
	}
}

}
