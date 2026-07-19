#include <cstdio>

#include "engine/replay_handler.hpp"
#include "itch/mapped_file.hpp"
#include "itch/parser.hpp"
#include "tests/reference_book.hpp"

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <itch-file> <symbol>\n", argv[0]);
        return 1;
    }

    matchline::MappedFile file{argv[1]};
    matchline::ReferenceBook book;
    matchline::ReplayHandler<matchline::ReferenceBook> handler{book, argv[2]};

    const auto consumed = matchline::parse(file.bytes(), handler);

    std::printf("bytes consumed: %zu\n", consumed);
    std::printf("file size: %zu\n", file.size());
    std::printf("live orders: %zu\n", book.live_orders());

    if (const auto bid = book.best_bid()) {
        std::printf("best bid ticks: %lld\n", static_cast<long long>(bid->ticks()));
    } else {
        std::printf("best bid:  (none)\n");
    }
    if (const auto ask = book.best_ask()) {
        std::printf("best ask ticks: %lld\n", static_cast<long long>(ask->ticks()));
    } else {
        std::printf("best ask:  (none)\n");
    }

    return 0;
}
