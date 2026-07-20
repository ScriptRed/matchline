#include "reference_book.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using matchline::OrderId;
using matchline::Price;
using matchline::Quantity;
using matchline::ReferenceBook;
using matchline::Side;

TEST(ReferenceBook, EmptyBookHasNoBestPrices) {
    ReferenceBook book;
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
    EXPECT_EQ(book.live_orders(), 0u);
}

TEST(ReferenceBook, AddSingleBidAndAsk) {
    ReferenceBook book;
    book.add(OrderId{1}, Side::Buy, Price{100}, Quantity{10});
    book.add(OrderId{2}, Side::Sell, Price{105}, Quantity{5});

    ASSERT_TRUE(book.best_bid().has_value());
    ASSERT_TRUE(book.best_ask().has_value());
    EXPECT_EQ(*book.best_bid(), Price{100});
    EXPECT_EQ(*book.best_ask(), Price{105});
    EXPECT_EQ(book.live_orders(), 2u);
}

TEST(ReferenceBook, BidsSortDescendingBestIsHighest) {
    ReferenceBook book;
    book.add(OrderId{1}, Side::Buy, Price{100}, Quantity{10});
    book.add(OrderId{2}, Side::Buy, Price{102}, Quantity{10});
    book.add(OrderId{3}, Side::Buy, Price{101}, Quantity{10});

    EXPECT_EQ(*book.best_bid(), Price{102});
}

TEST(ReferenceBook, AsksSortAscendingBestIsLowest) {
    ReferenceBook book;
    book.add(OrderId{1}, Side::Sell, Price{110}, Quantity{10});
    book.add(OrderId{2}, Side::Sell, Price{108}, Quantity{10});
    book.add(OrderId{3}, Side::Sell, Price{109}, Quantity{10});

    EXPECT_EQ(*book.best_ask(), Price{108});
}

TEST(ReferenceBook, CancelReducesQtyButOrderSurvives) {
    ReferenceBook book;
    book.add(OrderId{1}, Side::Buy, Price{100}, Quantity{100});
    book.cancel(OrderId{1}, Quantity{40});

    EXPECT_EQ(book.qty_at(Side::Buy, Price{100}), 60u);
    EXPECT_EQ(book.live_orders(), 1u);
}

TEST(ReferenceBook, ExecuteReducesQtySameAsCancel) {
    ReferenceBook book;
    book.add(OrderId{1}, Side::Buy, Price{100}, Quantity{100});
    book.execute(OrderId{1}, Quantity{30});

    EXPECT_EQ(book.qty_at(Side::Buy, Price{100}), 70u);
    EXPECT_EQ(book.live_orders(), 1u);
}

TEST(ReferenceBook, RemoveErasesOrderAndEmptiesLevel) {
    ReferenceBook book;
    book.add(OrderId{1}, Side::Buy, Price{100}, Quantity{10});
    book.remove(OrderId{1});

    EXPECT_EQ(book.live_orders(), 0u);
    EXPECT_FALSE(book.best_bid().has_value());
}

TEST(ReferenceBook, RemoveOneOrderLeavesLevelAliveIfOthersRemain) {
    ReferenceBook book;
    book.add(OrderId{1}, Side::Buy, Price{100}, Quantity{10});
    book.add(OrderId{2}, Side::Buy, Price{100}, Quantity{20});
    book.remove(OrderId{1});

    EXPECT_EQ(book.live_orders(), 1u);
    EXPECT_EQ(*book.best_bid(), Price{100});
    EXPECT_EQ(book.qty_at(Side::Buy, Price{100}), 20u);
}

TEST(ReferenceBook, QtyAtSumsAllOrdersAtLevel) {
    ReferenceBook book;
    book.add(OrderId{1}, Side::Buy, Price{100}, Quantity{10});
    book.add(OrderId{2}, Side::Buy, Price{100}, Quantity{15});

    EXPECT_EQ(book.qty_at(Side::Buy, Price{100}), 25u);
}

TEST(ReferenceBook, QtyAtUnknownLevelIsZero) {
    ReferenceBook book;
    EXPECT_EQ(book.qty_at(Side::Buy, Price{999}), 0u);
}

TEST(ReferenceBook, SideOfReturnsCorrectSide) {
    ReferenceBook book;
    book.add(OrderId{1}, Side::Sell, Price{100}, Quantity{10});
    EXPECT_EQ(book.side_of(OrderId{1}), Side::Sell);
}

TEST(ReferenceBook, ReduceMoreThanRestingSharesThrows) {
    ReferenceBook book;
    book.add(OrderId{1}, Side::Buy, Price{100}, Quantity{10});
    EXPECT_THROW(book.cancel(OrderId{1}, Quantity{11}), std::runtime_error);
}

TEST(ReferenceBook, RemoveUnknownOrderThrows) {
    ReferenceBook book;
    EXPECT_THROW(book.remove(OrderId{404}), std::out_of_range);
}

TEST(ReferenceBook, DuplicateAddThrows) {
    ReferenceBook book;
    book.add(OrderId{1}, Side::Buy, Price{100}, Quantity{10});
    EXPECT_THROW(book.add(OrderId{1}, Side::Sell, Price{200}, Quantity{5}), std::runtime_error);
}

TEST(ReferenceBook, BookNeverCrossesAcrossOperations) {
    ReferenceBook book;
    book.add(OrderId{1}, Side::Buy, Price{100}, Quantity{10});
    book.add(OrderId{2}, Side::Sell, Price{105}, Quantity{10});
    book.add(OrderId{3}, Side::Buy, Price{103}, Quantity{5});
    book.cancel(OrderId{1}, Quantity{5});
    book.remove(OrderId{2});
    book.add(OrderId{4}, Side::Sell, Price{104}, Quantity{5});

    ASSERT_TRUE(book.best_bid().has_value());
    ASSERT_TRUE(book.best_ask().has_value());
    EXPECT_LT(book.best_bid()->ticks(), book.best_ask()->ticks());
}
