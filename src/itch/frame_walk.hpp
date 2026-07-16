#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "util/byte_order.hpp"

namespace matchline {

struct FrameStats {
    std::array<uint64_t, 256> counts{};
    std::size_t final_offset = 0;
    bool truncated = false;
};

inline FrameStats count_messages(std::span<const std::byte> bytes) {
    FrameStats stats;
    std::size_t offset = 0;

    while (offset + 2 <= bytes.size()) {
        const uint16_t len = load_be16(bytes.data() + offset);
        offset += 2;
        if (offset + len > bytes.size()) {
            stats.truncated = true;
            break;
        }
        ++stats.counts[std::to_integer<uint8_t>(bytes[offset])];
        offset += len;
    }

    stats.final_offset = offset;
    return stats;
}

} 
