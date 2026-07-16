#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace matchline {

inline uint16_t load_be16(const std::byte* p) noexcept {
    uint16_t v;
    std::memcpy(&v, p, sizeof(v));
    return __builtin_bswap16(v);
}

inline uint32_t load_be32(const std::byte* p) noexcept {
    uint32_t v;
    std::memcpy(&v, p, sizeof(v));
    return __builtin_bswap32(v);
}

inline uint64_t load_be64(const std::byte* p) noexcept {
    uint64_t v;
    std::memcpy(&v, p, sizeof(v));
    return __builtin_bswap64(v);
}

inline uint64_t load_be48(const std::byte* p) noexcept {
    uint64_t v = 0;
    std::memcpy(reinterpret_cast<std::byte*>(&v) + 2, p, 6);
    return __builtin_bswap64(v);
}

}
