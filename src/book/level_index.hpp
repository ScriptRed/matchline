#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

namespace matchline {

// A two level bitset over occupied price ticks
// level 0 has one bit per tick
// level 1 has one summary bit per 64 ticks of level 0.

class LevelIndex {
public:
    explicit LevelIndex(std::size_t num_levels)
        : num_levels_{num_levels},
          level0_((num_levels + 63) / 64, 0),
          level1_((level0_.size() + 63) / 64, 0) {}

    void set(std::size_t index) {
        check_range(index);
        const std::size_t word0 = index / 64;
        level0_[word0] |= (uint64_t{1} << (index % 64));
        level1_[word0 / 64] |= (uint64_t{1} << (word0 % 64));
    }

    void clear(std::size_t index) {
        check_range(index);
        const std::size_t word0 = index / 64;
        level0_[word0] &= ~(uint64_t{1} << (index % 64));
        if (level0_[word0] == 0) {
            level1_[word0 / 64] &= ~(uint64_t{1} << (word0 % 64));
        }
    }

    std::optional<std::size_t> lowest() const {
        for (std::size_t w1 = 0; w1 < level1_.size(); ++w1) {
            if (level1_[w1] == 0) continue;
            const std::size_t bit1 = static_cast<std::size_t>(std::countr_zero(level1_[w1]));
            const std::size_t word0 = w1 * 64 + bit1;
            const std::size_t bit0 = static_cast<std::size_t>(std::countr_zero(level0_[word0]));
            return word0 * 64 + bit0;
        }
        return std::nullopt;
    }

    std::optional<std::size_t> highest() const {
        for (std::size_t i = level1_.size(); i-- > 0;) {
            if (level1_[i] == 0) continue;
            const std::size_t bit1 = 63 - static_cast<std::size_t>(std::countl_zero(level1_[i]));
            const std::size_t word0 = i * 64 + bit1;
            const std::size_t bit0 = 63 - static_cast<std::size_t>(std::countl_zero(level0_[word0]));
            return word0 * 64 + bit0;
        }
        return std::nullopt;
    }

private:
    void check_range(std::size_t index) const {
        if (index >= num_levels_) throw std::out_of_range("LevelIndex: index out of range");
    }

    std::size_t num_levels_;
    std::vector<uint64_t> level0_;
    std::vector<uint64_t> level1_;
};

}
