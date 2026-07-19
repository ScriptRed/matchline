#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

#include "core/types.hpp"

namespace matchline {

template <typename Book>
class ReplayHandler {
public:
    ReplayHandler(Book& book, std::string target_symbol)
        : book_{book}, target_symbol_{std::move(target_symbol)} {}

    void on_directory(uint16_t locate, Symbol sym) {
        if (sym.to_string() == target_symbol_) target_locate_ = locate;
    }

    void on_add(uint64_t /*ts*/, OrderId id, uint16_t locate, Side side, Quantity shares, Price price) {
        if (!target_locate_.has_value() || locate != *target_locate_) return;
        tracked_.insert(id);
        book_.add(id, side, price, shares);
    }

    void on_delete(uint64_t /*ts*/, OrderId id) {
        if (tracked_.erase(id) == 0) return;
        book_.remove(id);
    }

    void on_cancel(uint64_t /*ts*/, OrderId id, Quantity shares) {
        if (!tracked_.contains(id)) return;
        book_.cancel(id, shares);
    }

    void on_execute(uint64_t /*ts*/, OrderId id, Quantity shares) {
        if (!tracked_.contains(id)) return;
        book_.execute(id, shares);
    }

    void on_replace(uint64_t /*ts*/, OrderId old_id, OrderId new_id, Quantity shares, Price price) {
        if (tracked_.erase(old_id) == 0) return;
        const Side side = book_.side_of(old_id);
        book_.remove(old_id);
        tracked_.insert(new_id);
        book_.add(new_id, side, price, shares);
    }

private:
    Book& book_;
    std::string target_symbol_;
    std::optional<uint16_t> target_locate_;
    std::unordered_set<OrderId> tracked_;
};

}
