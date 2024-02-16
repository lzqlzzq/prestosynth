# prestosynth
prestosynth is a **presto\*** software synthesizer based on SoundFont specification.
# Limitation
* **prestosynth** is not intended to fulfill all of `SoundFont` specification but only to provide synthesization in **presto** speed.
* **prestosynth** is not intended to be a real-time synthesizer but to fully utilize parallelism.
* **prestosynth** will yield more memory than other synthsizers due to aggressive cache mechanism.
* `Modulator` of `Soundfont Standard` is not implemented yet, for its complex topology.
* `MIDI Controller` is not implemented yet.
# Implemented
- [x] Wavetable Oscillator
- [x] Sample Looping
- [x] Envelope Generator
- [x] Gain Amplifier
# TODO
- [ ] Better, variable pitch shifting
- [ ] Correct Release caculation
- [ ] LPF
- [ ] LFO
- [ ] Basic Reverb effect
- [ ] Basic Chrous effect
