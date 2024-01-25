#ifndef _SOUNDFONT_H
#define _SOUNDFONT_H

#include <system_error>
#include <string>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <ios>
#include <mio/mmap.hpp>
#include "math_util.h"


namespace prestosynth {

#define SF_META_CHUNK_TYPES                                   \
    SF_CHUNK_TYPE(RIFF)                                  \
    SF_CHUNK_TYPE(sfbk)                                  \
    SF_CHUNK_TYPE(LIST)                                  \

#define SF_CHUNK_TYPES                                   \
    SF_CHUNK_TYPE(INFO)                                  \
    SF_CHUNK_TYPE(sdta)                                  \
    SF_CHUNK_TYPE(pdta)

#define PDTA_SUB_CHUNK_TYPES                             \
    SF_CHUNK_TYPE(phdr)                                  \
    SF_CHUNK_TYPE(pbag)                                  \
    SF_CHUNK_TYPE(pmod)                                  \
    SF_CHUNK_TYPE(pgen)                                  \
    SF_CHUNK_TYPE(inst)                                  \
    SF_CHUNK_TYPE(ibag)                                  \
    SF_CHUNK_TYPE(imod)                                  \
    SF_CHUNK_TYPE(igen)                                  \
    SF_CHUNK_TYPE(shdr)

#define SF_DATA_CHUNK_TYPES                              \
    SF_CHUNK_TYPE(ifil)                                  \
    SF_CHUNK_TYPE(smpl)                                  \
    SF_CHUNK_TYPE(sm24)

#define SF_CHUNK_TYPE(name)                              \
const std::string name##_CHUNKID = std::string(#name);
SF_META_CHUNK_TYPES
SF_DATA_CHUNK_TYPES
SF_CHUNK_TYPES
PDTA_SUB_CHUNK_TYPES
#undef SF_CHUNK_TYPE

inline size_t read_le_bytes(uint8_t* cursor, size_t length) {
    size_t num = 0;

    for(size_t i = 0; i < length; i++) {
        num += (*(cursor + i)) << (8 * i);
    }

    return num;
};

inline std::string read_string_bytes(uint8_t* cursor, size_t length) {
    return std::string(reinterpret_cast<const char*>(cursor), length);
};


namespace soundfont_internal {

#define PERCUSSION_BANK 128

#pragma pack(1)
struct PresetHeader {
    char name[20];
    uint16_t presetNum;
    uint16_t bankNum;
    uint16_t bagIdx;
    uint32_t library;
    uint32_t genre;
    uint32_t morphology;
};  // phdr
static_assert(sizeof(PresetHeader) == 38);  // Not supported platform if assertation failed.
typedef PresetHeader phdrData;

struct Bag {
    uint16_t genIdx;
    uint16_t modIdx;
};  // pbag or ibag
static_assert(sizeof(Bag) == 4);
typedef Bag pbagData;
typedef Bag ibagData;

struct Instrument {
    char name[20];
    uint16_t bagIdx;
};  // inst
static_assert(sizeof(Instrument) == 22);
typedef Instrument instData;

enum GeneratorType : uint16_t {
    StartAddrsOffset = 0, // Offset Start Address
    EndAddrsOffset = 1, // Offset End Address 
    StartloopAddrsOffset = 2, // Offset Startloop Address
    EndloopAddrsOffset = 3, // Offset Endloop Address
    StartAddrsCoarseOffset = 4, // Offset Start Address Coarse
    ModLfoToPitch = 5, // Modulation LFO to Pitch 
    VibLfoToPitch = 6,  // Vibrato LFO to Pitch
    ModEnvToPitch = 7, // Modulation Envelope to Pitch 
    InitialFilterFc = 8, // Initial Filter Cutoff Frequency
    InitialFilterQ = 9, // Initial Filter Q
    ModLfoToFilterFc = 10, // Modulation LFO to Filter Cutoff Frequency
    ModEnvToFilterFc = 11, // Modulation Envelope to Filter Cutoff Frequency 
    EndAddrsCoarseOffset = 12, // Offset End Address Coarse
    ModLfoToVolume = 13, // Modulation LFO to Volume
    ChorusEffectsSend = 15, // Chorus Effects Send
    ReverbEffectsSend = 16, // Reverb Effects Send
    Pan = 17, // Pan
    DelayModLFO = 21, // Delay Modulation LFO   
    FreqModLFO = 22, // Frequency Modulation LFO
    DelayVibLFO = 23, // Delay Vibrato LFO
    FreqVibLFO = 24, // Frequency Vibrato LFO 
    DelayModEnv = 25, // Delay Modulation Envelope
    AttackModEnv = 26, // Attack Modulation Envelope
    HoldModEnv = 27, // Hold Modulation Envelope
    DecayModEnv = 28, // Decay Modulation Envelope
    SustainModEnv = 29, // Sustain Modulation Envelope
    ReleaseModEnv = 30, // Release Modulation Envelope
    KeynumToModEnvHold = 31, // Key Number to Modulation Envelope Hold  
    KeynumToModEnvDecay = 32, // Key Number to Modulation Envelope Decay
    DelayVolEnv = 33, // Delay Volume Envelope
    AttackVolEnv = 34,  // Attack Volume Envelope 
    HoldVolEnv = 35, // Hold Volume Envelope
    DecayVolEnv = 36, // Decay Volume Envelope
    SustainVolEnv = 37, // Sustain Volume Envelope 
    ReleaseVolEnv = 38, // Release Volume Envelope
    KeynumToVolEnvHold = 39, // Key Number to Volume Envelope Hold
    KeynumToVolEnvDecay = 40, // Key Number to Volume Envelope Decay 
    Instrument = 41, // Instrument Index
    KeyRange = 43, // Key Range  
    VelRange = 44, // Velocity Range  
    StartloopAddrsCoarseOffset = 45, // Offset Startloop Address Coarse 
    Keynum = 46, // Forced MIDI Key Number
    Velocity = 47, // Forced MIDI Velocity 
    InitialAttenuation = 48, // Initial Attenuation 
    EndloopAddrsCoarseOffset = 50, // Offset Endloop Address Coarse
    CoarseTune = 51, // Coarse Tune  
    FineTune = 52, // Fine Tune 
    SampleID = 53, // Sample Index 
    SampleModes = 54, // Sample Modes
    ScaleTuning = 56, // Scale Tuning 
    ExclusiveClass = 57, // Exclusive Class   
    OverridingRootKey = 58, // Overriding Root Key
    EndOper = 60  // End Operator
};

enum SrcPalette {
    None = 0, // No Controller
    NoteOnVel = 2, // Note-On Velocity
    NoteOnKey = 3, // Note-On Key Number 
    PolyPressure = 10, // Poly Pressure
    ChannelPressure = 13, // Channel Pressure
    PitchWheel = 14, // Pitch Wheel
    PitchWheelSens = 16, // Pitch Wheel Sensitivity
    Link = 127, // Link - NOT for Amount Source
};

enum SrcDirection {
    MinToMax = 0, // Min to Max
    MaxToMin = 1 // Max to Min    
};

enum SrcPolarity {
    Unipolar = 0, // Unipolar (0 to 1)
    Bipolar = 1 // Bipolar (-1 to 1)
};

enum SrcType {
    Linear = 0, // Linear
    Concave = 1, // Concave, output = log(sqrt(value^2)/(max value)^2)
    Convex = 2, // Convex 
    Switch = 3 // Switch
};

union ModulatorType {
    struct {
        unsigned type: 6;
        unsigned polarity: 1;
        unsigned direction: 1;
        unsigned CCflag: 1;
        unsigned index: 7;
    };
    uint16_t value;
};
static_assert(sizeof(ModulatorType) == 2);

enum TransformType : uint16_t {
    NoOpr = 0,
    AbsValue = 2
};

struct Modulator {
    ModulatorType modSrcOper;
    GeneratorType modDestOper;
    uint16_t modAmount;
    ModulatorType modAmtSrcOper;
    TransformType modTransOper;
};  // pmod or imod
static_assert(sizeof(Modulator) == 10);
typedef Modulator pmodData;
typedef Modulator imodData;

struct Generator {
    GeneratorType genOper;
    union {
        struct {
            uint8_t lowByte;
            uint8_t highByte;
        };
        int16_t sAmount;
        uint16_t uAmount;
    };
};  // pgen or igen
static_assert(sizeof(Generator) == 4);
typedef Generator pgenData;
typedef Generator igenData;

enum SampleType : uint16_t {
    mono = 0x0001,
    right = 0x0002,
    left = 0x0004,
    linked = 0x0008,
    romMono = 0x8001,
    romRight = 0x8002,
    romLeft = 0x8004,
    romLinked = 0x8008
};

struct SampleHeader {
    char name[20];
    uint32_t start;
    uint32_t end;
    uint32_t startloop;
    uint32_t endloop;
    uint32_t sampleRate;
    uint8_t originalPitch;
    int8_t pitchCorrection;
    uint16_t sampleLink;
    SampleType sampleType;
};  // shdr
static_assert(sizeof(SampleHeader) == 46);
typedef SampleHeader shdrData;

class SoundFontChunk {
protected:
    uint8_t* handler = nullptr;
    size_t size = 0;
    uint8_t* cursor(size_t offset);

public:
    friend class SoundFont;

    SoundFontChunk() = default;
    SoundFontChunk(uint8_t* handler, size_t size):
        handler(handler), size(size) {};
};

class SdtaChunk : public SoundFontChunk {
    using SoundFontChunk::SoundFontChunk;
};

class PdtaChunk : public SoundFontChunk {
private:
#define SF_CHUNK_TYPE(name)                              \
    name##Data* name##Handler = nullptr;                            \
    size_t name##Num = 0;
PDTA_SUB_CHUNK_TYPES
#undef SF_CHUNK_TYPE

public:
    using SoundFontChunk::SoundFontChunk;
    PdtaChunk(uint8_t* handler, size_t size);

#define SF_CHUNK_TYPE(name)                             \
    name##Data name(size_t index) const;                \
    size_t name##_num() const;
PDTA_SUB_CHUNK_TYPES
#undef SF_CHUNK_TYPE
};

class SoundFont {
protected:
    mio::basic_mmap_source<uint8_t> handler;
    uint16_t version = 0;
    SdtaChunk sdtaChunk;
    PdtaChunk pdtaChunk;

    uint8_t* cursor(size_t offset);
    uint16_t read_version(size_t offset, size_t maxSize);

public:
    SoundFont(const std::string &filepath);
    ~SoundFont();

    uint16_t get_version() const;
    SdtaChunk sdta() const;
    PdtaChunk pdta() const;
};

}

}

#endif
