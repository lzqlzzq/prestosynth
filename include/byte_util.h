#ifndef _BYTE_UTIL_H
#define _BYTE_UTIL_H

#include <string>
#include <climits>
#include <iostream>

namespace psynth {

inline std::string read_string_bytes(const uint8_t* cursor, size_t length) {
    return std::string(reinterpret_cast<const char*>(cursor), length);
};

template <typename T>
union BytesUnion {
    T value;
    uint8_t bytes[sizeof(T)];
};

template <typename T>
inline T swap_endian(T value) {
    static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

    BytesUnion<T> source, dest;
    source.value = value;

    for (size_t k = 0; k < sizeof(T); k++)
        dest.bytes[k] = source.bytes[sizeof(T) - k - 1];

    return dest.value;
};

template <typename T>
inline T read_le_bytes(const uint8_t* cursor) {
    return reinterpret_cast<const BytesUnion<T>*>(cursor)->value;
};

}

#endif
