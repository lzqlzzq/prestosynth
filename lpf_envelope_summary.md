# Low-Pass Filter (LPF) and Envelope Implementation Summary

This document summarizes common approaches for implementing Low-Pass Filters (LPF) and ADSR Envelopes in audio synthesis, based on analysis of TinySoundFont, FluidLite, and meltysynth.

## Low-Pass Filter (LPF)

LPFs are crucial for shaping the timbre of synthesized sounds by attenuating frequencies above a cutoff frequency.

**Common Implementation:**

*   **Type:** 2nd-Order IIR (Biquad) Filter is the standard choice. It provides a good balance between effectiveness and computational cost.
*   **Topology:** Direct Form I or Direct Form II (Transposed Direct Form II is common for numerical stability).
*   **Difference Equation (Direct Form I):**
    `y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]`
*   **State Variables:** Requires storing previous two input samples (`x[n-1]`, `x[n-2]`) and previous two output samples (`y[n-1]`, `y[n-2]`). (Or equivalent state for other forms like DF-II).

**Parameters:**

*   **Cutoff Frequency (Fc):** The frequency above which attenuation increases. Often modulated by envelopes or LFOs. Input units can vary (Hz, absolute cents like SF2). Needs conversion to a normalized frequency relative to the sample rate for coefficient calculation (e.g., `w = 2 * pi * fc_hz / sample_rate`).
*   **Resonance (Q):** Controls the sharpness of the filter peak at the cutoff frequency. Higher Q means a sharper peak. Input units can vary (linear factor, dB like SF2). Needs conversion for coefficient calculation. SF2 uses centibels (cB), often requiring conversion to linear scale and potentially adjustment (like the -3dB adjustment in FluidLite/meltysynth for Q=0dB).

**Coefficient Calculation:**

*   Coefficients (`b0`, `b1`, `b2`, `a1`, `a2`) depend on Fc, Q, and the sample rate.
*   Standard biquad formulas exist for low-pass filters. Example (from meltysynth):
    ```
    w = 2 * PI * Fc / Fs
    cosw = cos(w)
    alpha = sin(w) / (2 * Q) // Q needs appropriate scaling/conversion first

    b0 = (1 - cosw) / 2
    b1 = 1 - cosw
    b2 = (1 - cosw) / 2
    a0_coeff = 1 + alpha  // Note: a0_coeff here is the denominator term, not the final 'a0' coefficient in the difference equation format above
    a1_coeff = -2 * cosw
    a2_coeff = 1 - alpha

    // Final coefficients for difference equation (normalized by a0_coeff)
    final_b0 = b0 / a0_coeff
    final_b1 = b1 / a0_coeff
    final_b2 = b2 / a0_coeff
    final_a1 = a1_coeff / a0_coeff // Note the sign change convention difference
    final_a2 = a2_coeff / a0_coeff // Note the sign change convention difference
    ```
*   **Optimization:** Coefficients only need recalculation when Fc or Q changes significantly. If Fc/Q are modulated per sample, coefficients must be updated per sample. Can be bypassed if Fc is above the Nyquist frequency or a high threshold (e.g., > 20kHz).

## ADSR Envelope

Envelopes control how a sound parameter (like volume or filter cutoff) changes over time after a note is triggered. The ADSR (Attack, Decay, Sustain, Release) model is standard.

**Stages:**

1.  **Delay:** (Optional, from SF2 spec) Time before the attack phase starts. Output is zero.
2.  **Attack:** Time taken for the level to rise from zero to the peak level (usually 1.0). Typically linear.
3.  **Hold:** (Optional, from SF2 spec) Time the envelope stays at the peak level after attack.
4.  **Decay:** Time taken for the level to fall from the peak level to the sustain level. Typically exponential.
5.  **Sustain:** The level maintained as long as the note is held *after* the decay phase. It's a level, not a time.
6.  **Release:** Time taken for the level to fall from its current level (usually the sustain level, or the level during attack/decay if released early) to zero after the note is released. Typically exponential.

**Parameters:**

*   Attack Time/Rate
*   Decay Time/Rate
*   Sustain Level (0.0 to 1.0)
*   Release Time/Rate
*   (Optional) Delay Time, Hold Time

**Implementation:**

*   **State Machine:** The envelope progresses through stages (Delay -> Attack -> Hold -> Decay -> Sustain -> Release -> Off).
*   **State Variables:** Current stage, time/samples elapsed in the current stage, current output value.
*   **Time Conversion:** Input times (often in seconds or timecents like SF2) need conversion to sample counts based on the sample rate. Timecents require `pow(2, timecents / 1200.0)` conversion.
*   **Calculations:**
    *   **Attack:** Linear slope (e.g., `slope = 1.0 / attack_samples`). `value += slope`.
    *   **Decay/Release:** Exponential decay. Can be implemented using multiplication: `value *= multiplier` where `multiplier = exp(log(target_level / start_level) / decay_samples)`. Or using simpler approximations or lookup tables. TinySoundFont/meltysynth use `exp(slope * time_in_stage)`. FluidLite seems to use linear segments approximating the curve.
    *   **Sustain:** Output value remains constant at the sustain level.
*   **Processing:** Typically processed per audio block. The state is updated, and the envelope value is calculated for each sample in the block.

**Key Considerations from References:**

*   **FluidLite/TinySoundFont:** Handle SF2 timecents conversion for times. Use key/velocity scaling for envelope times (e.g., `KeyNumToHold`, `KeyNumToDecay`).
*   **meltysynth:** Clear separation between Volume and Modulation envelopes. Uses `SoundFontMath.ExpCutoff` for exponential decay/release, providing a value slightly above zero for the target to avoid issues with `log(0)`.
*   **All:** Need careful state management, especially handling stage transitions and note release during different stages. 