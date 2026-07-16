#include <chrono>
#include <cstdio>
#include <span>

#include "itch/frame_walk.hpp"
#include "itch/mapped_file.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <itch-file>\n", argv[0]);
        return 1;
    }

    matchline::MappedFile file{argv[1]};
    std::span<const std::byte> bytes = file.bytes();

    const auto start = std::chrono::steady_clock::now();
    const matchline::FrameStats stats = matchline::count_messages(bytes);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const double seconds = std::chrono::duration<double>(elapsed).count();

    if (stats.truncated) {
        std::fprintf(stderr, "truncated message at offset %zu (%zu bytes remain)\n", stats.final_offset,
                     bytes.size() - stats.final_offset);
    }

    uint64_t total = 0;
    std::printf("%-4s %s\n", "type", "count");
    for (int i = 0; i < 256; ++i) {
        if (stats.counts[static_cast<std::size_t>(i)] == 0) continue;
        std::printf("%-4c %llu\n", static_cast<char>(i),
                    static_cast<unsigned long long>(stats.counts[static_cast<std::size_t>(i)]));
        total += stats.counts[static_cast<std::size_t>(i)];
    }

    std::printf("\ntotal messages: %llu\n", static_cast<unsigned long long>(total));
    std::printf("final offset:   %zu\n", stats.final_offset);
    std::printf("file size:      %zu\n", bytes.size());
    std::printf("bytes left:     %zu\n", bytes.size() - stats.final_offset);

    const double mb = static_cast<double>(bytes.size()) / (1024.0 * 1024.0);
    std::printf("elapsed:        %.3f s\n", seconds);
    std::printf("throughput:     %.1f MB/s\n", mb / seconds);

    return 0;
}
