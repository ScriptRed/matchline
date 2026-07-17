#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

namespace matchline {

enum class Side : uint8_t { Buy, Sell };

// ITCH prices are 4 byte unsigned integers with four implied decimal places
// 1234567 = $123.4567 
class Price {
public:
    constexpr explicit Price(int64_t ticks) noexcept : ticks_{ticks} {}
    constexpr int64_t ticks() const noexcept { return ticks_; }
    friend constexpr auto operator<=>(Price, Price) = default;

private:
    int64_t ticks_;
};

class Quantity {
public:
    constexpr explicit Quantity(uint32_t shares) noexcept : shares_{shares} {}
    constexpr uint32_t value() const noexcept { return shares_; }
    friend constexpr auto operator<=>(Quantity, Quantity) = default;

private:
    uint32_t shares_;
};

class OrderId {
public:
    constexpr explicit OrderId(uint64_t id) noexcept : id_{id} {}
    constexpr uint64_t value() const noexcept { return id_; }
    friend constexpr auto operator<=>(OrderId, OrderId) = default;

private:
    uint64_t id_;
};

// Stock symbols are 8 ASCII bytes
class Symbol {
public:
    constexpr Symbol() noexcept : packed_{0} {}
    explicit Symbol(const std::byte* raw) noexcept { std::memcpy(&packed_, raw, sizeof(packed_)); }

    friend constexpr auto operator<=>(Symbol, Symbol) = default;

    [[nodiscard]] std::string to_string() const {
        char chars[8];
        std::memcpy(chars, &packed_, sizeof(chars));
        std::size_t len = 8;
        while (len > 0 && chars[len - 1] == ' ') --len;
        return std::string(chars, len);
    }

private:
    uint64_t packed_;
};

}

template <>
struct std::hash<matchline::OrderId> {
    std::size_t operator()(matchline::OrderId id) const noexcept {
        return std::hash<uint64_t>{}(id.value());
    }
};
