#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "core/types.hpp"

namespace matchline {

inline constexpr uint32_t kNullIndex = std::numeric_limits<uint32_t>::max();

struct Order {
    OrderId id{0};
    uint32_t qty = 0;
    uint32_t prev = kNullIndex;
    uint32_t next = kNullIndex;
    int32_t level = -1;
    Side side{};
};
static_assert(sizeof(Order) <= 32);

// a flat fixed capacity pool of Order slots
class OrderPool {
public:
    explicit OrderPool(std::size_t capacity) : slots_(capacity) {
        for (std::size_t i = 0; i + 1 < capacity; ++i) {
            slots_[i].next = static_cast<uint32_t>(i + 1);
        }
        free_head_ = capacity > 0 ? 0 : kNullIndex;
    }

    uint32_t acquire() {
        if (free_head_ == kNullIndex) throw std::runtime_error("OrderPool: capacity exceeded");
        const uint32_t index = free_head_;
        free_head_ = slots_[index].next;
        slots_[index] = Order{};
        return index;
    }

    void release(uint32_t index) {
        slots_[index] = Order{};
        slots_[index].next = free_head_;
        free_head_ = index;
    }

    Order& operator[](uint32_t index) { return slots_[index]; }
    const Order& operator[](uint32_t index) const { return slots_[index]; }

private:
    std::vector<Order> slots_;
    uint32_t free_head_ = kNullIndex;
};

}
