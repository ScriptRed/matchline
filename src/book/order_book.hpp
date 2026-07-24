#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

#include "book/level_index.hpp"
#include "book/order_map.hpp"
#include "book/order_pool.hpp"
#include "book/price_level.hpp"
#include "core/types.hpp"

namespace matchline {

class OrderBook {
public:
    OrderBook(Price base, std::size_t num_levels, std::size_t order_capacity)
        : base_{base},
          num_levels_{num_levels},
          bid_index_{num_levels},
          ask_index_{num_levels},
          bid_levels_(num_levels),
          ask_levels_(num_levels),
          pool_{order_capacity},
          order_map_{next_pow2(order_capacity * 2)} {}

    void add(OrderId id, Side side, Price px, Quantity qty) {
        if (order_map_.find(id).has_value()) {
            throw std::runtime_error("OrderBook::add: duplicate order id");
        }
        const std::size_t level = level_of(px);

        const uint32_t index = pool_.acquire();
        Order& order = pool_[index];
        order.id = id;
        order.qty = qty.value();
        order.side = side;
        order.level = static_cast<int32_t>(level);

        if (side == Side::Buy) {
            level_push_back(bid_levels_[level], pool_, index);
            bid_index_.set(level);
        } else {
            level_push_back(ask_levels_[level], pool_, index);
            ask_index_.set(level);
        }
        order_map_.insert(id, index);
    }

    void cancel(OrderId id, Quantity qty) { reduce(id, qty.value()); }
    void execute(OrderId id, Quantity qty) { reduce(id, qty.value()); }

    void remove(OrderId id) {
        const uint32_t index = find_index(id);
        const Order& order = pool_[index];
        const auto level = static_cast<std::size_t>(order.level);
        if (order.side == Side::Buy) {
            level_erase(bid_levels_[level], pool_, index);
            if (bid_levels_[level].empty()) bid_index_.clear(level);
        } else {
            level_erase(ask_levels_[level], pool_, index);
            if (ask_levels_[level].empty()) ask_index_.clear(level);
        }
        order_map_.erase(id);
        pool_.release(index);
    }

    Side side_of(OrderId id) const { return pool_[find_index(id)].side; }

    std::optional<Price> best_bid() const {
        const auto level = bid_index_.highest();
        if (!level) return std::nullopt;
        return price_at(*level);
    }

    std::optional<Price> best_ask() const {
        const auto level = ask_index_.lowest();
        if (!level) return std::nullopt;
        return price_at(*level);
    }

    uint64_t qty_at(Side side, Price px) const {
        const std::size_t level = level_of(px);
        return (side == Side::Buy) ? bid_levels_[level].total_qty : ask_levels_[level].total_qty;
    }

    std::size_t live_orders() const { return order_map_.size(); }

private:
    // a cancel never removes the order
    void reduce(OrderId id, uint32_t qty) {
        const uint32_t index = find_index(id);
        Order& order = pool_[index];
        if (qty > order.qty) throw std::runtime_error("OrderBook::reduce: qty exceeds resting shares");
        order.qty -= qty;
        PriceLevel& level = (order.side == Side::Buy) ? bid_levels_[static_cast<std::size_t>(order.level)]
                                                        : ask_levels_[static_cast<std::size_t>(order.level)];
        level.total_qty -= qty;
    }

    uint32_t find_index(OrderId id) const {
        const auto index = order_map_.find(id);
        if (!index) throw std::out_of_range("OrderBook: unknown order id");
        return *index;
    }

    std::size_t level_of(Price px) const {
        const int64_t offset = px.ticks() - base_.ticks();
        if (offset < 0 || static_cast<std::size_t>(offset) >= num_levels_) {
            throw std::out_of_range("OrderBook: price outside configured range");
        }
        return static_cast<std::size_t>(offset);
    }

    Price price_at(std::size_t level) const {
        return Price{base_.ticks() + static_cast<int64_t>(level)};
    }

    static std::size_t next_pow2(std::size_t n) {
        std::size_t p = 1;
        while (p < n) p *= 2;
        return p;
    }

    Price base_;
    std::size_t num_levels_;
    LevelIndex bid_index_;
    LevelIndex ask_index_;
    std::vector<PriceLevel> bid_levels_;
    std::vector<PriceLevel> ask_levels_;
    OrderPool pool_;
    OrderMap order_map_;
};

}
