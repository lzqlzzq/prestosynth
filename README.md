# prestosynth
prestosynth is a software synthesizer based on SoundFont specification, in **presto** speed.
# Cloning
For the big object of soundfont, use `GIT_LFS_SKIP_SMUDGE=1` to avoid cloning it:
```
GIT_LFS_SKIP_SMUDGE=1 git clone https://github.com/lzqlzzq/prestosynth.git
```
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
# To be Implemented
- [ ] Better, variable pitch shifting (OLA-based or delay-line based algorithm)
- [ ] Basic LPF
- [ ] Basic LFO
- [ ] Basic Reverb effect
- [ ] Basic Chrous effect
