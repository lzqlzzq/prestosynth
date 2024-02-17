# prestosynth
`prestosynth` is a **presto\*** software synthesizer based on SoundFont specification.
```
*) presto[Italian]: as fast as you can
```
# Motivation
As the rapid progress of symbolic music generation, there is a growing need of fast server-side audio rendering from symbolic music. I proposed `prestosynth` following these principles:
* For the environment of servers, it should be self-contained and portable.
* For the characteristic of servers, parallelism and multiplexing should be utilized extremly, while memory cost is negligible.
* For the simplicity, it should utilize out-of-the-box standards.
# Limitation
* **prestosynth** is not intended to fulfill all of `SoundFont` specification but to provide synthesization in **presto** speed.
* **prestosynth** is not intended to be a real-time synthesizer but to extremly utilize parallelism.
* **prestosynth** will yield more memory than other synthsizers due to aggressive cache mechanism.
* `Modulator` of `Soundfont Standard` is not implemented yet, for its complex topology.
* `MIDI Controller` is not implemented yet.
# Implemented
- [x] Wavetable Oscillator
- [x] Sample Looping
- [x] Envelope Generator
- [x] Gain Amplifier
- [x] Low Pass Filter
# TODO
- [ ] Conditioning pitch bend with LFO, MOD
- [ ] Conditioning volume with LFO, MOD
- [ ] Conditioning LPF with LFO, MOD
- [ ] Correct Release caculation
- [ ] Basic Reverb effect
- [ ] Basic Chrous effect
- [ ] Reorganize parallel paradigm
