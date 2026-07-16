#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>

#include "core/types.hpp"
#include "itch/messages.hpp"
#include "util/byte_order.hpp"

namespace matchline {

template <typename Handler>
std::size_t parse(std::span<const std::byte> data, Handler& handler) {
    std::size_t offset = 0;

    while (offset + 2 <= data.size()) {
        const uint16_t len = load_be16(data.data() + offset);
        const std::size_t msg_start = offset + 2;
        if (msg_start + len > data.size()) break;  // truncated final message
        if (len == 0) throw std::runtime_error("parse: zero-length message");

        const std::byte* msg = data.data() + msg_start;
        const auto type = std::to_integer<uint8_t>(msg[0]);
        if (len != kMessageLength[type]) {
            throw std::runtime_error("parse: declared length disagrees with message type");
        }

        const uint16_t locate = load_be16(msg + header::kStockLocate);
        const uint64_t ts = load_be48(msg + header::kTimestamp);

        switch (static_cast<char>(type)) {
            case 'A':
            case 'F': {
                const OrderId id{load_be64(msg + add_order::kOrderRefNum)};
                const Side side =
                    (msg[add_order::kBuySellIndicator] == std::byte{'B'}) ? Side::Buy : Side::Sell;
                const Quantity shares{load_be32(msg + add_order::kShares)};
                const Price price{static_cast<int64_t>(load_be32(msg + add_order::kPrice))};
                handler.on_add(ts, id, locate, side, shares, price);
                break;
            }
            case 'D': {
                const OrderId id{load_be64(msg + order_delete::kOrderRefNum)};
                handler.on_delete(ts, id);
                break;
            }
            case 'X': {
                const OrderId id{load_be64(msg + order_cancel::kOrderRefNum)};
                const Quantity shares{load_be32(msg + order_cancel::kCancelledShares)};
                handler.on_cancel(ts, id, shares);
                break;
            }
            case 'E':
            case 'C': {
                const OrderId id{load_be64(msg + order_executed::kOrderRefNum)};
                const Quantity shares{load_be32(msg + order_executed::kExecutedShares)};
                handler.on_execute(ts, id, shares);
                break;
            }
            case 'U': {
                const OrderId old_id{load_be64(msg + order_replace::kOriginalOrderRefNum)};
                const OrderId new_id{load_be64(msg + order_replace::kNewOrderRefNum)};
                const Quantity shares{load_be32(msg + order_replace::kShares)};
                const Price price{static_cast<int64_t>(load_be32(msg + order_replace::kPrice))};
                handler.on_replace(ts, old_id, new_id, shares, price);
                break;
            }
            case 'R': {
                const Symbol sym{msg + stock_directory::kStock};
                handler.on_directory(locate, sym);
                break;
            }
            default:
                break;
        }

        offset = msg_start + len;
    }

    return offset;
}

}
