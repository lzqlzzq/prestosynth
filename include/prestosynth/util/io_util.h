//
// Created by nhy on 2024/2/13.
//
#pragma once
#ifndef IO_UTIL_H
#define IO_UTIL_H

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#ifdef _WIN32
#include <Windows.h>

namespace psynth {
namespace details {
inline std::wstring toUtf16(const std::string& str) {
    std::wstring ret;
    const int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
    if (len > 0) {
        ret.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &ret[0], len);
    }
    return ret;
}
} // details
} // psynth
#endif

namespace psynth {
// cross-platform file open with utf-8 support
inline FILE* open_file(const std::string& filePath, const std::string& mode) {
#ifndef _WIN32
    FILE* fp = fopen(filePath.c_str(), mode.c_str());
    if(fp == nullptr) {
        throw std::runtime_error("File not found file: " + filePath);
    }
#else   // deal with utf-8 path on windows
    FILE* fp = nullptr;
    const errno_t err = _wfopen_s(&fp, details::toUtf16(filePath).c_str(), details::toUtf16(mode).c_str());
    if (err != 0) {
        throw std::runtime_error("File not found file (error:" + std::to_string(err) + "): " + filePath);
    }
#endif
    return fp;
}
} // psynth

#endif //IO_UTIL_H
