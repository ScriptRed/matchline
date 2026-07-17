#include <cstddef>
#include <cstdint>
#include <exception>
#include <span>

#include "itch/parser.hpp"

namespace {

// A no-op handler: the fuzz target only cares whether parse() ever crashes
struct NoopHandler {
    void on_add(uint64_t, matchline::OrderId, uint16_t, matchline::Side, matchline::Quantity,
                matchline::Price) {}
    void on_delete(uint64_t, matchline::OrderId) {}
    void on_cancel(uint64_t, matchline::OrderId, matchline::Quantity) {}
    void on_execute(uint64_t, matchline::OrderId, matchline::Quantity) {}
    void on_replace(uint64_t, matchline::OrderId, matchline::OrderId, matchline::Quantity,
                     matchline::Price) {}
    void on_directory(uint16_t, matchline::Symbol) {}
};

}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    NoopHandler handler;
    const std::span<const std::byte> bytes(reinterpret_cast<const std::byte*>(data), size);
    try {
        matchline::parse(bytes, handler);
    } catch (const std::exception&) {
        // rejecting malformed input via a thrown exception is the parser
    }
    return 0;
}
