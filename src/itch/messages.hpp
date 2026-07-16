#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// ITCH 5.0 field offsets
namespace matchline {

inline constexpr std::array<uint8_t, 256> kMessageLength = [] {
    std::array<uint8_t, 256> t{};
    t['S'] = 12; t['R'] = 39; t['H'] = 25; t['Y'] = 20;
    t['L'] = 26; t['V'] = 35; t['W'] = 12; t['K'] = 28;
    t['A'] = 36; t['F'] = 40; t['E'] = 31; t['C'] = 36;
    t['X'] = 23; t['D'] = 19; t['U'] = 35; t['P'] = 44;
    t['Q'] = 40; t['B'] = 19; t['I'] = 50; t['N'] = 20;
    return t;
}();

// common 11 byte header shared by every message type
namespace header {
inline constexpr std::size_t kType = 0;
inline constexpr std::size_t kStockLocate = 1;
inline constexpr std::size_t kTrackingNumber = 3;
inline constexpr std::size_t kTimestamp = 5;
inline constexpr std::size_t kSize = 11;
}

// 'A' (36 bytes) and 'F' (40 bytes)

namespace add_order {
inline constexpr std::size_t kOrderRefNum = 11;
inline constexpr std::size_t kBuySellIndicator = 19;
inline constexpr std::size_t kShares = 20;
inline constexpr std::size_t kStock = 24;
inline constexpr std::size_t kPrice = 32;
}

namespace order_delete {  // 'D'
inline constexpr std::size_t kOrderRefNum = 11;
}

namespace order_cancel {  // 'X'
inline constexpr std::size_t kOrderRefNum = 11;
inline constexpr std::size_t kCancelledShares = 19;
}

// 'E' (31 bytes) and 'C' (36 bytes)
namespace order_executed {
inline constexpr std::size_t kOrderRefNum = 11;
inline constexpr std::size_t kExecutedShares = 19;
}

namespace order_replace {  // 'U'
inline constexpr std::size_t kOriginalOrderRefNum = 11;
inline constexpr std::size_t kNewOrderRefNum = 19;
inline constexpr std::size_t kShares = 27;
inline constexpr std::size_t kPrice = 31;
}

namespace stock_directory {  // 'R'
inline constexpr std::size_t kStock = 11;
}

}
