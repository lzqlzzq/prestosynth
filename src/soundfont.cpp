#include "soundfont.h"
#include "byte_util.h"
#include <iostream>

namespace prestosynth {

namespace soundfont_internal {

uint8_t* SoundFont::cursor(size_t offset) {
    return const_cast<uint8_t*>(handler.begin()) + offset;
};

void SoundFont::read_INFO_chunk(size_t offset, size_t chunkSize) {
    size_t endOffset = offset + chunkSize;
    while(!version) {
        size_t chunkSize = read_le_bytes<uint32_t>(cursor(offset + 4));
        if(offset + chunkSize > endOffset)
            throw std::ios_base::failure("Unexcepted EOF in INFO chunk!");

        if(read_string_bytes(cursor(offset), 4) == ifil_CHUNKID) {
            version = read_le_bytes<uint32_t>(cursor(offset + 8));
            break;
        }

        offset + chunkSize + 8;
    }
    if(version != 2 && version != 3)
        throw std::ios_base::failure("Only sf2 or sf3 supported!");
};

void SoundFont::read_sdta_chunk(size_t offset, size_t chunkSize) {
    size_t endOffset = offset + chunkSize;
    while(!smplHandler) {
        size_t chunkSize = read_le_bytes<uint32_t>(cursor(offset + 4));
        if(offset + chunkSize > endOffset)
            throw std::ios_base::failure("Unexcepted EOF in sdta chunk!");

        if(read_string_bytes(cursor(offset), 4) == smpl_CHUNKID) {
            smplHandler = cursor(offset + 4);
            smplSize = read_le_bytes<uint32_t>(cursor(offset + 4));
            break;
        }

        offset + chunkSize + 8;
    }
};

void SoundFont::read_pdta_chunk(size_t offset, size_t chunkSize) {
    size_t endOffset = offset + chunkSize;
    while(!(
            #define SF_CHUNK_TYPE(name) name##Num &&
            PDTA_SUB_CHUNK_TYPES
            #undef SF_CHUNK_TYPE
            true)) {
        size_t chunkSize = read_le_bytes<uint32_t>(cursor(offset + 4));

        if(offset + chunkSize > endOffset)
            throw std::ios_base::failure("Unexcepted EOF in pdta chunk!");

        #define SF_CHUNK_TYPE(name)                                                        \
        if(read_string_bytes(cursor(offset), 4) == name##_CHUNKID) {                       \
            if(chunkSize % sizeof(name##Data))                                             \
                throw std::ios_base::failure("Not valid " + name##_CHUNKID + " chunk!");   \
            name##Handler = reinterpret_cast<name##Data*>(cursor(offset + 8));           \
            name##Num = read_le_bytes<uint32_t>(cursor(offset + 4)) / sizeof(name##Data);         \
        }
        PDTA_SUB_CHUNK_TYPES
        #undef SF_CHUNK_TYPE

        offset += chunkSize + 8;
    }
};

SoundFont::SoundFont(const std::string &filepath) {
    std::error_code err;
    handler.map(filepath, err);

    if(err)
        throw std::ios_base::failure(std::string("Error mapping file: ") + filepath + std::string("\n") + err.message());

    if(read_string_bytes(cursor(0), 4) != RIFF_CHUNKID)
        throw std::ios_base::failure("Not a valid RIFF file!");

    size_t fileSize = read_le_bytes<uint32_t>(cursor(4));

    if(fileSize > handler.size() - 8)
        throw std::ios_base::failure("Unexcepted EOF!");

    if(read_string_bytes(cursor(8), 4) != sfbk_CHUNKID)
        throw std::ios_base::failure("Not a valid sfbk file!");

    size_t offset = 12;

#define SF_CHUNK_TYPE(name)                              \
    bool name##Processed = false;
SF_CHUNK_TYPES
#undef SF_CHUNK_TYPE

    while(!(
    #define SF_CHUNK_TYPE(name) \
        name##Processed &&
    SF_CHUNK_TYPES
    #undef SF_CHUNK_TYPE
    true)) {
        if(read_string_bytes(cursor(offset), 4) != LIST_CHUNKID)
            throw std::ios_base::failure("LIST chunk excepted!");

        size_t chunkSize = read_le_bytes<uint32_t>(cursor(offset + 4));

        if(offset + chunkSize > fileSize)
            throw std::ios_base::failure("Unexcepted EOF!");

    #define SF_CHUNK_TYPE(name) \
        if(read_string_bytes(cursor(offset + 8), 4) == name##_CHUNKID) { \
            read_##name##_chunk(offset + 12, chunkSize); \
            name##Processed = true; \
        }
    SF_CHUNK_TYPES
    #undef SF_CHUNK_TYPE

        offset += chunkSize + 8;
    }
};

SoundFont::~SoundFont() {
    handler.unmap();
};

uint16_t SoundFont::get_version() const {
    return version;
};

#define SF_CHUNK_TYPE(name)                                                               \
name##Data SoundFont::name(size_t index) const {                                          \
    if(index > name##Num)                                                                 \
        std::out_of_range("Index is out of range!");                                      \
    return *(name##Handler + index);                                                      \
};                                                                                        \
                                                                                          \
size_t SoundFont::name##_num() const {                                                    \
    return name##Num;                                                                     \
};
PDTA_SUB_CHUNK_TYPES
#undef SF_CHUNK_TYPE

}

}
