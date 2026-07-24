#pragma once

#include <cstdint>

#include "book/order_pool.hpp"

namespace matchline {

// doubly-linked list to manage orders
struct PriceLevel {
    uint32_t head = kNullIndex;
    uint32_t tail = kNullIndex;
    uint32_t count = 0;
    uint64_t total_qty = 0;

    bool empty() const noexcept { return count == 0; }
};

inline void level_push_back(PriceLevel& level, OrderPool& pool, uint32_t index) {
    Order& order = pool[index];
    order.prev = level.tail;
    order.next = kNullIndex;
    if (level.tail != kNullIndex) {
        pool[level.tail].next = index;
    } else {
        level.head = index;
    }
    level.tail = index;
    ++level.count;
    level.total_qty += order.qty;
}

inline void level_erase(PriceLevel& level, OrderPool& pool, uint32_t index) {
    const Order& order = pool[index];
    if (order.prev != kNullIndex) {
        pool[order.prev].next = order.next;
    } else {
        level.head = order.next;
    }
    if (order.next != kNullIndex) {
        pool[order.next].prev = order.prev;
    } else {
        level.tail = order.prev;
    }
    --level.count;
    level.total_qty -= order.qty;
}

}
