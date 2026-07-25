#pragma once

#include <bit>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <vector>

#include "core/types.hpp"

namespace matchline {

// OrderId -> pool-index map:
class OrderMap {
public:
    explicit OrderMap(std::size_t capacity_pow2)
        : mask_{capacity_pow2 - 1}, slots_(capacity_pow2) {
        if (!std::has_single_bit(capacity_pow2)) {
            throw std::invalid_argument("OrderMap: capacity must be a power of two");
        }
    }

    void insert(OrderId id, uint32_t pool_index) {
        std::size_t i = hash(id) & mask_;
        for (std::size_t probes = 0; probes <= mask_; ++probes) {
            if (slots_[i].state != State::kOccupied) {
                slots_[i] = {State::kOccupied, id, pool_index};
                ++size_;
                return;
            }
            i = (i + 1) & mask_;
        }
        throw std::runtime_error("OrderMap: full");
    }

    std::optional<uint32_t> find(OrderId id) const {
        std::size_t i = hash(id) & mask_;
        for (std::size_t probes = 0; probes <= mask_; ++probes) {
            if (slots_[i].state == State::kEmpty) return std::nullopt;
            if (slots_[i].state == State::kOccupied && slots_[i].id == id) return slots_[i].index;
            i = (i + 1) & mask_;
        }
        return std::nullopt;
    }

    void erase(OrderId id) {
        std::size_t i = hash(id) & mask_;
        for (std::size_t probes = 0; probes <= mask_; ++probes) {
            if (slots_[i].state == State::kEmpty) return;
            if (slots_[i].state == State::kOccupied && slots_[i].id == id) {
                slots_[i].state = State::kTombstone;
                --size_;
                return;
            }
            i = (i + 1) & mask_;
        }
    }

    std::size_t size() const noexcept { return size_; }

private:
    enum class State : uint8_t { kEmpty, kOccupied, kTombstone };
    struct Slot {
        State state = State::kEmpty;
        OrderId id{0};
        uint32_t index = 0;
    };

    static std::size_t hash(OrderId id) noexcept { return std::hash<OrderId>{}(id); }

    std::size_t mask_;
    std::vector<Slot> slots_;
    std::size_t size_ = 0;
};

}
