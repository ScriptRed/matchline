#include "itch/parser.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/types.hpp"

namespace {

void put_be16(std::byte* p, uint16_t v) {
    p[0] = static_cast<std::byte>(v >> 8);
    p[1] = static_cast<std::byte>(v & 0xFF);
}

void put_be32(std::byte* p, uint32_t v) {
    for (int i = 0; i < 4; ++i) p[i] = static_cast<std::byte>((v >> (24 - 8 * i)) & 0xFF);
}

void put_be48(std::byte* p, uint64_t v) {
    for (int i = 0; i < 6; ++i) p[i] = static_cast<std::byte>((v >> (40 - 8 * i)) & 0xFF);
}

void put_be64(std::byte* p, uint64_t v) {
    for (int i = 0; i < 8; ++i) p[i] = static_cast<std::byte>((v >> (56 - 8 * i)) & 0xFF);
}

std::vector<std::byte> frame(const std::vector<std::byte>& body) {
    std::vector<std::byte> out(body.size() + 2);
    put_be16(out.data(), static_cast<uint16_t>(body.size()));
    std::memcpy(out.data() + 2, body.data(), body.size());
    return out;
}

std::vector<std::byte> make_add_order(char type, uint64_t order_id, char side, uint32_t shares,
                                       uint32_t price_ticks, uint16_t locate = 7,
                                       uint64_t ts = 123456789012) {
    const std::size_t len = (type == 'F') ? 40 : 36;
    std::vector<std::byte> body(len, std::byte{0});
    body[0] = static_cast<std::byte>(type);
    put_be16(body.data() + 1, locate);
    put_be48(body.data() + 5, ts);
    put_be64(body.data() + 11, order_id);
    body[19] = static_cast<std::byte>(side);
    put_be32(body.data() + 20, shares);
    put_be32(body.data() + 32, price_ticks);
    return frame(body);
}

std::vector<std::byte> make_delete(uint64_t order_id, uint16_t locate = 7) {
    std::vector<std::byte> body(19, std::byte{0});
    body[0] = std::byte{'D'};
    put_be16(body.data() + 1, locate);
    put_be64(body.data() + 11, order_id);
    return frame(body);
}

std::vector<std::byte> make_cancel(uint64_t order_id, uint32_t shares, uint16_t locate = 7) {
    std::vector<std::byte> body(23, std::byte{0});
    body[0] = std::byte{'X'};
    put_be16(body.data() + 1, locate);
    put_be64(body.data() + 11, order_id);
    put_be32(body.data() + 19, shares);
    return frame(body);
}

std::vector<std::byte> make_executed(char type, uint64_t order_id, uint32_t shares,
                                      uint16_t locate = 7) {
    const std::size_t len = (type == 'C') ? 36 : 31;
    std::vector<std::byte> body(len, std::byte{0});
    body[0] = static_cast<std::byte>(type);
    put_be16(body.data() + 1, locate);
    put_be64(body.data() + 11, order_id);
    put_be32(body.data() + 19, shares);
    return frame(body);
}

std::vector<std::byte> make_replace(uint64_t old_id, uint64_t new_id, uint32_t shares,
                                     uint32_t price_ticks, uint16_t locate = 7) {
    std::vector<std::byte> body(35, std::byte{0});
    body[0] = std::byte{'U'};
    put_be16(body.data() + 1, locate);
    put_be64(body.data() + 11, old_id);
    put_be64(body.data() + 19, new_id);
    put_be32(body.data() + 27, shares);
    put_be32(body.data() + 31, price_ticks);
    return frame(body);
}

std::vector<std::byte> make_directory(uint16_t locate, const std::string& symbol) {
    std::vector<std::byte> body(39, std::byte{0});
    body[0] = std::byte{'R'};
    put_be16(body.data() + 1, locate);
    for (std::size_t i = 0; i < 8; ++i) {
        const char c = (i < symbol.size()) ? symbol[i] : ' ';
        body[11 + i] = static_cast<std::byte>(c);
    }
    return frame(body);
}

std::vector<std::byte> concat(std::initializer_list<std::vector<std::byte>> parts) {
    std::vector<std::byte> out;
    for (const auto& part : parts) out.insert(out.end(), part.begin(), part.end());
    return out;
}

struct RecordingHandler {
    struct Add {
        uint64_t ts;
        matchline::OrderId id;
        uint16_t locate;
        matchline::Side side;
        matchline::Quantity shares;
        matchline::Price price;
    };
    struct Delete {
        uint64_t ts;
        matchline::OrderId id;
    };
    struct Cancel {
        uint64_t ts;
        matchline::OrderId id;
        matchline::Quantity shares;
    };
    struct Execute {
        uint64_t ts;
        matchline::OrderId id;
        matchline::Quantity shares;
    };
    struct Replace {
        uint64_t ts;
        matchline::OrderId old_id;
        matchline::OrderId new_id;
        matchline::Quantity shares;
        matchline::Price price;
    };
    struct Directory {
        uint16_t locate;
        matchline::Symbol sym;
    };

    std::vector<Add> adds;
    std::vector<Delete> deletes;
    std::vector<Cancel> cancels;
    std::vector<Execute> executes;
    std::vector<Replace> replaces;
    std::vector<Directory> directories;

    void on_add(uint64_t ts, matchline::OrderId id, uint16_t locate, matchline::Side side,
                matchline::Quantity shares, matchline::Price price) {
        adds.push_back({ts, id, locate, side, shares, price});
    }
    void on_delete(uint64_t ts, matchline::OrderId id) { deletes.push_back({ts, id}); }
    void on_cancel(uint64_t ts, matchline::OrderId id, matchline::Quantity shares) {
        cancels.push_back({ts, id, shares});
    }
    void on_execute(uint64_t ts, matchline::OrderId id, matchline::Quantity shares) {
        executes.push_back({ts, id, shares});
    }
    void on_replace(uint64_t ts, matchline::OrderId old_id, matchline::OrderId new_id,
                     matchline::Quantity shares, matchline::Price price) {
        replaces.push_back({ts, old_id, new_id, shares, price});
    }
    void on_directory(uint16_t locate, matchline::Symbol sym) { directories.push_back({locate, sym}); }
};

}

TEST(Parser, AddOrderNoMpid) {
    RecordingHandler h;
    const auto data = make_add_order('A', /*order_id=*/42, 'B', /*shares=*/100, /*price_ticks=*/1234567);
    const auto consumed = matchline::parse(data, h);

    ASSERT_EQ(h.adds.size(), 1u);
    EXPECT_EQ(h.adds[0].id.value(), 42u);
    EXPECT_EQ(h.adds[0].locate, 7);
    EXPECT_EQ(h.adds[0].side, matchline::Side::Buy);
    EXPECT_EQ(h.adds[0].shares.value(), 100u);
    EXPECT_EQ(h.adds[0].price.ticks(), 1234567);
    EXPECT_EQ(h.adds[0].ts, 123456789012u);
    EXPECT_EQ(consumed, data.size());
}

TEST(Parser, AddOrderWithMpidIgnoresAttribution) {
    RecordingHandler h;
    const auto data = make_add_order('F', 7, 'S', 50, 999999);
    matchline::parse(data, h);

    ASSERT_EQ(h.adds.size(), 1u);
    EXPECT_EQ(h.adds[0].id.value(), 7u);
    EXPECT_EQ(h.adds[0].side, matchline::Side::Sell);
}

TEST(Parser, DeleteOrder) {
    RecordingHandler h;
    const auto data = make_delete(99);
    matchline::parse(data, h);

    ASSERT_EQ(h.deletes.size(), 1u);
    EXPECT_EQ(h.deletes[0].id.value(), 99u);
}

TEST(Parser, CancelReducesShares) {
    RecordingHandler h;
    const auto data = make_cancel(5, 30);
    matchline::parse(data, h);

    ASSERT_EQ(h.cancels.size(), 1u);
    EXPECT_EQ(h.cancels[0].shares.value(), 30u);
}

TEST(Parser, ExecutedWithoutPrice) {
    RecordingHandler h;
    const auto data = make_executed('E', 3, 10);
    matchline::parse(data, h);

    ASSERT_EQ(h.executes.size(), 1u);
    EXPECT_EQ(h.executes[0].id.value(), 3u);
    EXPECT_EQ(h.executes[0].shares.value(), 10u);
}

TEST(Parser, ExecutedWithPriceRoutesToSameCallback) {
    RecordingHandler h;
    const auto data = make_executed('C', 3, 10);
    matchline::parse(data, h);

    ASSERT_EQ(h.executes.size(), 1u);
    EXPECT_EQ(h.executes[0].shares.value(), 10u);
}

TEST(Parser, ReplaceCarriesBothOrderIds) {
    RecordingHandler h;
    const auto data = make_replace(/*old=*/1, /*new=*/2, 40, 555);
    matchline::parse(data, h);

    ASSERT_EQ(h.replaces.size(), 1u);
    EXPECT_EQ(h.replaces[0].old_id.value(), 1u);
    EXPECT_EQ(h.replaces[0].new_id.value(), 2u);
    EXPECT_EQ(h.replaces[0].shares.value(), 40u);
}

TEST(Parser, StockDirectoryMapsLocateToSymbol) {
    RecordingHandler h;
    const auto data = make_directory(11, "AAPL");
    matchline::parse(data, h);

    ASSERT_EQ(h.directories.size(), 1u);
    EXPECT_EQ(h.directories[0].locate, 11);
    EXPECT_EQ(h.directories[0].sym.to_string(), "AAPL");
}

TEST(Parser, MultipleMessagesInOneBufferConsumesAllOfIt) {
    RecordingHandler h;
    const auto data = concat({make_add_order('A', 1, 'B', 10, 100), make_delete(1), make_cancel(2, 5)});
    const auto consumed = matchline::parse(data, h);

    EXPECT_EQ(consumed, data.size());
    EXPECT_EQ(h.adds.size(), 1u);
    EXPECT_EQ(h.deletes.size(), 1u);
    EXPECT_EQ(h.cancels.size(), 1u);
}

TEST(Parser, TruncatedFinalMessageStopsWithoutThrowing) {
    RecordingHandler h;
    const auto full = make_add_order('A', 1, 'B', 10, 100);
    const std::vector<std::byte> data(full.begin(), full.begin() + 10);

    const auto consumed = matchline::parse(data, h);

    EXPECT_EQ(consumed, 0u);
    EXPECT_TRUE(h.adds.empty());
}

TEST(Parser, LengthDisagreeingWithTableThrows) {
    auto data = make_add_order('A', 1, 'B', 10, 100);
    data[1] = std::byte{35};
    RecordingHandler h;

    EXPECT_THROW(matchline::parse(data, h), std::runtime_error);
}

TEST(Parser, ZeroLengthMessageThrows) {
    const std::vector<std::byte> data = {std::byte{0}, std::byte{0}};
    RecordingHandler h;

    EXPECT_THROW(matchline::parse(data, h), std::runtime_error);
}
