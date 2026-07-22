#include "engine/replay_handler.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "itch/parser.hpp"
#include "reference_book.hpp"

namespace {

void put_be16(std::byte* p, uint16_t v) {
    p[0] = static_cast<std::byte>(v >> 8);
    p[1] = static_cast<std::byte>(v & 0xFF);
}

void put_be32(std::byte* p, uint32_t v) {
    for (int i = 0; i < 4; ++i) p[i] = static_cast<std::byte>((v >> (24 - 8 * i)) & 0xFF);
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

std::vector<std::byte> make_add(uint16_t locate, uint64_t order_id, char side, uint32_t shares,
                                 uint32_t price_ticks) {
    std::vector<std::byte> body(36, std::byte{0});
    body[0] = std::byte{'A'};
    put_be16(body.data() + 1, locate);
    put_be64(body.data() + 11, order_id);
    body[19] = static_cast<std::byte>(side);
    put_be32(body.data() + 20, shares);
    put_be32(body.data() + 32, price_ticks);
    return frame(body);
}

std::vector<std::byte> make_cancel(uint64_t order_id, uint32_t shares) {
    std::vector<std::byte> body(23, std::byte{0});
    body[0] = std::byte{'X'};
    put_be64(body.data() + 11, order_id);
    put_be32(body.data() + 19, shares);
    return frame(body);
}

std::vector<std::byte> make_delete(uint64_t order_id) {
    std::vector<std::byte> body(19, std::byte{0});
    body[0] = std::byte{'D'};
    put_be64(body.data() + 11, order_id);
    return frame(body);
}

std::vector<std::byte> make_replace(uint64_t old_id, uint64_t new_id, uint32_t shares,
                                     uint32_t price_ticks) {
    std::vector<std::byte> body(35, std::byte{0});
    body[0] = std::byte{'U'};
    put_be64(body.data() + 11, old_id);
    put_be64(body.data() + 19, new_id);
    put_be32(body.data() + 27, shares);
    put_be32(body.data() + 31, price_ticks);
    return frame(body);
}

std::vector<std::byte> concat(std::initializer_list<std::vector<std::byte>> parts) {
    std::vector<std::byte> out;
    for (const auto& part : parts) out.insert(out.end(), part.begin(), part.end());
    return out;
}

}

using matchline::OrderId;
using matchline::Price;
using matchline::ReferenceBook;
using matchline::ReplayHandler;
using matchline::Side;

TEST(ReplayHandler, FiltersOutOrdersFromOtherSymbols) {
    ReferenceBook book;
    ReplayHandler<ReferenceBook> handler{book, "AAPL"};

    const auto data = concat({
        make_directory(1, "AAPL"),
        make_directory(2, "MSFT"),
        make_add(/*locate=*/1, /*id=*/100, 'B', 50, 1000000),   // AAPL bid, tracked
        make_add(/*locate=*/2, /*id=*/200, 'B', 999, 5000000),  // MSFT bid, ignored
        make_add(/*locate=*/1, /*id=*/101, 'S', 30, 1005000),   // AAPL ask, tracked
    });

    const auto consumed = matchline::parse(data, handler);

    EXPECT_EQ(consumed, data.size());
    EXPECT_EQ(book.live_orders(), 2u);
    EXPECT_EQ(*book.best_bid(), Price{1000000});
    EXPECT_EQ(*book.best_ask(), Price{1005000});
}

TEST(ReplayHandler, CancelAndDeleteOnlyAffectTrackedOrders) {
    ReferenceBook book;
    ReplayHandler<ReferenceBook> handler{book, "AAPL"};

    const auto data = concat({
        make_directory(1, "AAPL"),
        make_directory(2, "MSFT"),
        make_add(1, 100, 'B', 50, 1000000),
        make_add(2, 200, 'B', 999, 5000000),
        make_cancel(100, 10),
        make_cancel(200, 10),
        make_delete(200),
    });

    matchline::parse(data, handler);

    EXPECT_EQ(book.live_orders(), 1u);
    EXPECT_EQ(book.qty_at(Side::Buy, Price{1000000}), 40u);
}

TEST(ReplayHandler, ReplacePreservesSideAndRetagsTrackedId) {
    ReferenceBook book;
    ReplayHandler<ReferenceBook> handler{book, "AAPL"};

    const auto data = concat({
        make_directory(1, "AAPL"),
        make_add(1, 101, 'S', 30, 1005000),
        make_replace(/*old=*/101, /*new=*/102, /*shares=*/25, /*price=*/1004000),
    });

    matchline::parse(data, handler);

    EXPECT_EQ(book.live_orders(), 1u);
    EXPECT_EQ(book.side_of(OrderId{102}), Side::Sell);
    EXPECT_EQ(*book.best_ask(), Price{1004000});

    const auto stray_cancel = make_cancel(101, 1);
    EXPECT_NO_THROW(matchline::parse(stray_cancel, handler));
}
