#ifndef _SOUNDFONT_INTERNAL_H
#define _SOUNDFONT_INTERNAL_H

#include <map>
#include <array>
#include <string>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <mio/mmap.hpp>
#include "prestosynth/util/audio_util.h"


namespace psynth::sf_internal {

#define SF_META_CHUNK_TYPES                              \
    SF_CHUNK_TYPE(RIFF)                                  \
    SF_CHUNK_TYPE(sfbk)                                  \
    SF_CHUNK_TYPE(LIST)

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

// (name, index, default)
#define SF_GENERATOR_TYPES \
    SF_GENERATOR_TYPE(StartAddrsOffset, 0, 0) \
    SF_GENERATOR_TYPE(EndAddrsOffset, 1, 0) \
    SF_GENERATOR_TYPE(StartloopAddrsOffset, 2, 0) \
    SF_GENERATOR_TYPE(EndloopAddrsOffset, 3, 0) \
    SF_GENERATOR_TYPE(StartAddrsCoarseOffset, 4, 0) \
    SF_GENERATOR_TYPE(ModLfoToPitch, 5, 0) \
    SF_GENERATOR_TYPE(VibLfoToPitch, 6, 0) \
    SF_GENERATOR_TYPE(ModEnvToPitch, 7, 0) \
    SF_GENERATOR_TYPE(InitialFilterFc, 8, 13500) \
    SF_GENERATOR_TYPE(InitialFilterQ, 9, 0) \
    SF_GENERATOR_TYPE(ModLfoToFilterFc, 10, 0) \
    SF_GENERATOR_TYPE(ModEnvToFilterFc, 11, 0) \
    SF_GENERATOR_TYPE(EndAddrsCoarseOffset, 12, 0) \
    SF_GENERATOR_TYPE(ModLfoToVolume, 13, 0) \
    SF_GENERATOR_TYPE(unused1, 14, 0) \
    SF_GENERATOR_TYPE(ChorusEffectsSend, 15, 0) \
    SF_GENERATOR_TYPE(ReverbEffectsSend, 16, 0) \
    SF_GENERATOR_TYPE(Pan, 17, 0) \
    SF_GENERATOR_TYPE(unused2, 18, 0) \
    SF_GENERATOR_TYPE(unused3, 19, 0) \
    SF_GENERATOR_TYPE(unused4, 20, 0) \
    SF_GENERATOR_TYPE(DelayModLFO, 21, -12000) \
    SF_GENERATOR_TYPE(FreqModLFO, 22, 0) \
    SF_GENERATOR_TYPE(DelayVibLFO, 23, -12000) \
    SF_GENERATOR_TYPE(FreqVibLFO, 24, 0) \
    SF_GENERATOR_TYPE(DelayModEnv, 25, -12000) \
    SF_GENERATOR_TYPE(AttackModEnv, 26, -12000) \
    SF_GENERATOR_TYPE(HoldModEnv, 27, -12000) \
    SF_GENERATOR_TYPE(DecayModEnv, 28, -12000) \
    SF_GENERATOR_TYPE(SustainModEnv, 29, 0) \
    SF_GENERATOR_TYPE(ReleaseModEnv, 30, -12000) \
    SF_GENERATOR_TYPE(KeynumToModEnvHold, 31, 0) \
    SF_GENERATOR_TYPE(KeynumToModEnvDecay, 32, 0) \
    SF_GENERATOR_TYPE(DelayVolEnv, 33, -12000) \
    SF_GENERATOR_TYPE(AttackVolEnv, 34, -12000) \
    SF_GENERATOR_TYPE(HoldVolEnv, 35, -12000) \
    SF_GENERATOR_TYPE(DecayVolEnv, 36, -12000) \
    SF_GENERATOR_TYPE(SustainVolEnv, 37, 0) \
    SF_GENERATOR_TYPE(ReleaseVolEnv, 38, -12000) \
    SF_GENERATOR_TYPE(KeynumToVolEnvHold, 39, 0) \
    SF_GENERATOR_TYPE(KeynumToVolEnvDecay, 40, 0) \
    SF_GENERATOR_TYPE(Instrument, 41, 0) \
    SF_GENERATOR_TYPE(reserved1, 42, 0) \
    SF_GENERATOR_TYPE(KeyRange, 43, 0x7f00) \
    SF_GENERATOR_TYPE(VelRange, 44, 0x7f00) \
    SF_GENERATOR_TYPE(StartloopAddrsCoarseOffset, 45, 0) \
    SF_GENERATOR_TYPE(Keynum, 46, -1) \
    SF_GENERATOR_TYPE(Velocity, 47, -1) \
    SF_GENERATOR_TYPE(InitialAttenuation, 48, 0) \
    SF_GENERATOR_TYPE(reserved2, 49, 0) \
    SF_GENERATOR_TYPE(EndloopAddrsCoarseOffset, 50, 0) \
    SF_GENERATOR_TYPE(CoarseTune, 51, 0) \
    SF_GENERATOR_TYPE(FineTune, 52, 0) \
    SF_GENERATOR_TYPE(SampleID, 53, 0) \
    SF_GENERATOR_TYPE(SampleModes, 54, 0) \
    SF_GENERATOR_TYPE(reserved3, 55, 0) \
    SF_GENERATOR_TYPE(ScaleTuning, 56, 100) \
    SF_GENERATOR_TYPE(ExclusiveClass, 57, 0) \
    SF_GENERATOR_TYPE(OverridingRootKey, 58, -1) \
    SF_GENERATOR_TYPE(unused5, 59, 0) \
    SF_GENERATOR_TYPE(EndOper, 60, 0)

enum GeneratorType : uint16_t {
#define SF_GENERATOR_TYPE(name, index, default) name = index,
    SF_GENERATOR_TYPES
#undef SF_GENERATOR_TYPE
};

union GenAmount {
    int16_t sAmount;
    uint16_t uAmount;
    struct {
        uint8_t lowByte;
        uint8_t highByte;
    };
};
static_assert(sizeof(GenAmount) == 2);

struct Generator {
    GeneratorType genOper;
    GenAmount amount;
};  // pgen or igen
static_assert(sizeof(Generator) == 4);
typedef Generator pgenData;
typedef Generator igenData;

typedef std::array<GenAmount, 61> GeneratorPack;

constexpr GeneratorPack defaultGenPack {
#define SF_GENERATOR_TYPE(name, index, defaultVal) defaultVal,
    SF_GENERATOR_TYPES
#undef SF_GENERATOR_TYPE
};

enum LoopMode {
    NoLoop = 0,
    Coutinuous = 1,
    Unused = 2,
    ToEnd = 3,
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
        uint8_t type: 6;
        uint8_t polarity: 1;
        uint8_t direction: 1;
        uint8_t CCflag: 1;
        uint8_t index: 7;
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
    uint32_t startOffset;
    uint32_t endOffset;
    uint32_t startLoop;
    uint32_t endLoop;
    uint32_t sampleRate;
    uint8_t originalPitch;
    int8_t pitchCorrection;
    uint16_t sampleLink;
    SampleType sampleType;

    inline size_t len() const { return endOffset - startOffset; };
};  // shdr
static_assert(sizeof(SampleHeader) == 46);
typedef SampleHeader shdrData;
#pragma pack()

class SoundFont {
protected:
    mio::basic_mmap_source<uint8_t> handler;
    uint16_t _version = 0;

    std::mutex cacheMtx;
    std::map<uint32_t, AudioData> sampleCache;

    uint8_t* smplHandler = nullptr;
    size_t smplSize = 0;

#define SF_CHUNK_TYPE(name)                              \
    name##Data* name##Handler = nullptr;                 \
    size_t name##Num = 0;
PDTA_SUB_CHUNK_TYPES
#undef SF_CHUNK_TYPE

    uint8_t* cursor(size_t offset);
    void read_INFO_chunk(size_t offset, size_t chunkSize);
    void read_sdta_chunk(size_t offset, size_t chunkSize);
    void read_pdta_chunk(size_t offset, size_t chunkSize);

public:
    SoundFont() = default;
    SoundFont(const std::string &filepath);
    ~SoundFont();

    SoundFont& operator=(const SoundFont& other);

    uint16_t version() const;
    AudioData sample(uint32_t startOffset, 
        uint32_t length,
        uint16_t originalSampleRate,
        uint32_t targetSampleRate,
        uint8_t quality);  // 0-4

#define SF_CHUNK_TYPE(name)                             \
    name##Data name(size_t index) const;                \
    size_t name##_num() const;
PDTA_SUB_CHUNK_TYPES
#undef SF_CHUNK_TYPE
};

}

#endif
