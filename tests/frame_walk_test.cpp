#include "itch/frame_walk.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

// Builds one wire format message: a 2 byte big endian length prefix followed
// by total_len bytes
std::vector<std::byte> make_message(char type, uint16_t total_len) {
    std::vector<std::byte> msg(static_cast<std::size_t>(total_len) + 2, std::byte{0});
    msg[0] = static_cast<std::byte>(total_len >> 8);
    msg[1] = static_cast<std::byte>(total_len & 0xFF);
    msg[2] = static_cast<std::byte>(type);
    return msg;
}

std::vector<std::byte> concat(std::initializer_list<std::vector<std::byte>> parts) {
    std::vector<std::byte> out;
    for (const auto& part : parts) out.insert(out.end(), part.begin(), part.end());
    return out;
}

}

TEST(FrameWalk, EmptyBufferProducesNoMessages) {
    const auto stats = matchline::count_messages({});

    EXPECT_EQ(stats.final_offset, 0u);
    EXPECT_FALSE(stats.truncated);
    for (auto count : stats.counts) EXPECT_EQ(count, 0u);
}

TEST(FrameWalk, CountsEachMessageTypeAndConsumesWholeBuffer) {
    const auto data = concat({make_message('S', 12), make_message('A', 36), make_message('D', 19)});
    const auto stats = matchline::count_messages(data);

    EXPECT_EQ(stats.counts[static_cast<unsigned char>('S')], 1u);
    EXPECT_EQ(stats.counts[static_cast<unsigned char>('A')], 1u);
    EXPECT_EQ(stats.counts[static_cast<unsigned char>('D')], 1u);
    EXPECT_EQ(stats.final_offset, data.size());
    EXPECT_FALSE(stats.truncated);
}

TEST(FrameWalk, RepeatedTypeIsCountedMultipleTimes) {
    const auto data = concat({make_message('A', 36), make_message('A', 36), make_message('A', 36)});
    const auto stats = matchline::count_messages(data);

    EXPECT_EQ(stats.counts[static_cast<unsigned char>('A')], 3u);
    EXPECT_EQ(stats.final_offset, data.size());
}

TEST(FrameWalk, TruncatedFinalMessageStopsWithoutReadingPastBuffer) {
    const auto full = concat({make_message('S', 12), make_message('A', 36)});
    const std::vector<std::byte> data(full.begin(), full.begin() + 14 + 5);

    const auto stats = matchline::count_messages(data);

    EXPECT_TRUE(stats.truncated);
    EXPECT_EQ(stats.counts[static_cast<unsigned char>('S')], 1u);
    EXPECT_EQ(stats.counts[static_cast<unsigned char>('A')], 0u);
    EXPECT_EQ(stats.final_offset, 16u);
}

TEST(FrameWalk, DanglingLengthPrefixWithNoPayloadIsTruncated) {
    const std::vector<std::byte> data = {std::byte{0}, std::byte{36}};
    const auto stats = matchline::count_messages(data);

    EXPECT_TRUE(stats.truncated);
    EXPECT_EQ(stats.final_offset, 2u);
}
