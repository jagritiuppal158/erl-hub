// BinaryIO.h  --  UNIT IV: tiny helpers for binary streams (std::fstream)
#ifndef BINARYIO_H
#define BINARYIO_H

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

#include "Exceptions.h"

namespace ReliefOps::BinaryIO {

template <typename T>
void writePod(std::ostream& os, const T& value) {
    os.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
void readPod(std::istream& is, T& value) {
    is.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!is) throw PersistenceException("Unexpected end of file while reading binary data");
}

inline void writeString(std::ostream& os, const std::string& s) {
    std::uint32_t n = static_cast<std::uint32_t>(s.size());
    writePod(os, n);
    os.write(s.data(), static_cast<std::streamsize>(n));
}

inline std::string readString(std::istream& is) {
    std::uint32_t n = 0;
    readPod(is, n);
    if (n > 100000) throw PersistenceException("Corrupt file: string length is unreasonable");
    std::string s(n, '\0');
    if (n > 0) {
        is.read(&s[0], static_cast<std::streamsize>(n));
        if (!is) throw PersistenceException("Unexpected end of file while reading a string");
    }
    return s;
}

}  // namespace ReliefOps::BinaryIO

#endif
