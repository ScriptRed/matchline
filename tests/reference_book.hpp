#pragma once

#include <algorithm>
#include <cstdint>
#include <list>
#include <map>
#include <optional>
#include <stdexcept>
#include <unordered_map>

#include "core/types.hpp"

namespace matchline {

class ReferenceBook {
public:
    void add(OrderId id, Side side, Price px, Quantity qty) {
        const auto [it, inserted] = orders_.emplace(id, Order{side, px, qty.value()});
        if (!inserted) throw std::runtime_error("ReferenceBook::add: duplicate order id");
        if (side == Side::Buy) {
            bids_[px].push_back(id);
        } else {
            asks_[px].push_back(id);
        }
    }

    void cancel(OrderId id, Quantity qty) { reduce(id, qty.value()); }
    void execute(OrderId id, Quantity qty) { reduce(id, qty.value()); }

    void remove(OrderId id) {
        const Order order = orders_.at(id);
        orders_.erase(id);
        if (order.side == Side::Buy) {
            erase_from_level(bids_, order.px, id);
        } else {
            erase_from_level(asks_, order.px, id);
        }
    }

    Side side_of(OrderId id) const { return orders_.at(id).side; }

    std::optional<Price> best_bid() const {
        if (bids_.empty()) return std::nullopt;
        return bids_.begin()->first;
    }

    std::optional<Price> best_ask() const {
        if (asks_.empty()) return std::nullopt;
        return asks_.begin()->first;
    }

    uint64_t qty_at(Side side, Price px) const {
        if (side == Side::Buy) return qty_at_impl(bids_, px);
        return qty_at_impl(asks_, px);
    }

    std::size_t live_orders() const { return orders_.size(); }

private:
    struct Order {
        Side side;
        Price px;
        uint32_t qty;
    };

    void reduce(OrderId id, uint32_t qty) {
        Order& order = orders_.at(id);
        if (qty > order.qty) {
            throw std::runtime_error("ReferenceBook::reduce: qty exceeds resting shares");
        }
        order.qty -= qty;
    }

    template <typename Levels>
    static void erase_from_level(Levels& levels, Price px, OrderId id) {
        auto level_it = levels.find(px);
        auto& ids = level_it->second;
        ids.erase(std::find(ids.begin(), ids.end(), id));
        if (ids.empty()) levels.erase(level_it);
    }

    template <typename Levels>
    uint64_t qty_at_impl(const Levels& levels, Price px) const {
        const auto level_it = levels.find(px);
        if (level_it == levels.end()) return 0;
        uint64_t total = 0;
        for (OrderId id : level_it->second) total += orders_.at(id).qty;
        return total;
    }

    std::map<Price, std::list<OrderId>, std::greater<>> bids_;
    std::map<Price, std::list<OrderId>> asks_;
    std::unordered_map<OrderId, Order> orders_;
};

}
