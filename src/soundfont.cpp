#include "soundfont.h"

#include<iostream>

namespace prestosynth {

namespace soundfont_internal {

uint8_t* SoundFontChunk::cursor(size_t offset) {
    return handler + offset;
}

PdtaChunk::PdtaChunk(uint8_t* ptr, size_t size): SoundFontChunk(ptr, size) {
    size_t offset = 0;
    while(!(
            #define SF_CHUNK_TYPE(name) name##Num &&
            PDTA_SUB_CHUNK_TYPES
            #undef SF_CHUNK_TYPE
            true)) {
        size_t chunkSize = read_le_bytes(cursor(offset + 4), 4);

        if(offset + chunkSize > size)
            throw std::ios_base::failure("Unexcepted EOF in pdta chunk!");

        #define SF_CHUNK_TYPE(name)                                                        \
        if(read_string_bytes(cursor(offset), 4) == name##_CHUNKID) {                       \
            if(chunkSize % sizeof(name##Data))                                             \
                throw std::ios_base::failure("Not valid " + name##_CHUNKID + " chunk!");   \
            name##Offset = offset + 8;                                                     \
            name##Num = read_le_bytes(cursor(offset + 4), 4) / sizeof(name##Data);         \
        }
        PDTA_SUB_CHUNK_TYPES
        #undef SF_CHUNK_TYPE
        offset += chunkSize + 8;
    }
};


#define SF_CHUNK_TYPE(name)                                                               \
name##Data PdtaChunk::name(size_t index) const {                                          \
    if(index > name##Num)                                                                 \
        std::out_of_range("Index is out of range!");                                      \
    return *(reinterpret_cast<name##Data*>(handler + name##Offset) + index);              \
};                                                                                        \
                                                                                          \
size_t PdtaChunk::name##_num() const {                                                    \
    return name##Num;                                                                     \
};
PDTA_SUB_CHUNK_TYPES
#undef SF_CHUNK_TYPE

uint8_t* SoundFont::cursor(size_t offset) {
    return const_cast<uint8_t*>(handler.begin()) + offset;
};

uint16_t SoundFont::read_version(size_t offset, size_t maxSize) {
    while(!version) {
        size_t chunkSize = read_le_bytes(cursor(offset + 4), 4);
        if(offset + chunkSize > maxSize)
            throw std::ios_base::failure("Unexcepted EOF in INFO chunk!");

        if(read_string_bytes(cursor(offset), 4) == ifil_CHUNKID)
            return read_le_bytes(cursor(offset + 8), 4);

        offset + chunkSize + 8;
    }
    return -1;
};

SoundFont::SoundFont(const std::string &filepath) {
    std::error_code err;
    handler.map(filepath, err);

    if(err)
        throw std::ios_base::failure(std::string("Error mapping file: ") + filepath + std::string("\n") + err.message());

    if(read_string_bytes(cursor(0), 4) != RIFF_CHUNKID)
        throw std::ios_base::failure("Not a valid RIFF file!");

    size_t fileSize = read_le_bytes(cursor(4), 4);

    if(fileSize > handler.size() - 8)
        throw std::ios_base::failure("Unexcepted EOF!");

    if(read_string_bytes(cursor(8), 4) != sfbk_CHUNKID)
        throw std::ios_base::failure("Not a valid sfbk file!");

    size_t offset = 12;

    while(!(version &&
        sdtaChunk.cursor(0) &&
        pdtaChunk.cursor(0))) {
        size_t chunkSize = read_le_bytes(cursor(offset + 4), 4);

        if(offset + chunkSize > fileSize)
            throw std::ios_base::failure("Unexcepted EOF!");

        if(read_string_bytes(cursor(offset + 8), 4) == INFO_CHUNKID)
            version = read_version(offset + 12, chunkSize);
        if(read_string_bytes(cursor(offset + 8), 4) == sdta_CHUNKID)
            sdtaChunk = SdtaChunk(cursor(offset + 12), chunkSize);
        if(read_string_bytes(cursor(offset + 8), 4) == pdta_CHUNKID)
            pdtaChunk = PdtaChunk(cursor(offset + 12), chunkSize);

        offset += chunkSize + 8;
    }
};

SoundFont::~SoundFont() {
    handler.unmap();
};

uint16_t SoundFont::get_version() const {
    return version;
};

SdtaChunk SoundFont::sdta() const {
    return sdtaChunk;
};

PdtaChunk SoundFont::pdta() const {
    return pdtaChunk;
};

}

}
