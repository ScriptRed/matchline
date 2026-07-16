#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstddef>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace matchline {

class MappedFile {
public:
    //opens and memory maps a file
    explicit MappedFile(const std::filesystem::path& path) {
        int fd = ::open(path.c_str(), O_RDONLY); 
        if (fd == -1) {
            throw std::runtime_error("MappedFile: failed to open " + path.string());
        }

        struct stat st {};
        if (::fstat(fd, &st) == -1) { // get file size and metadata
            ::close(fd);
            throw std::runtime_error("MappedFile: fstat failed for " + path.string());
        }
        size_ = static_cast<std::size_t>(st.st_size);

        if (size_ == 0) { // reject empty files
            ::close(fd);
            throw std::runtime_error("MappedFile: empty file " + path.string());
        }

        // map the file into memory 
        void* mapped = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd, 0);
        ::close(fd);  // fd no longer needed after mmap
        if (mapped == MAP_FAILED) {
            throw std::runtime_error("MappedFile: mmap failed for " + path.string());
        }

        ::madvise(mapped, size_, MADV_SEQUENTIAL); // file will be read sequentially (optimise caching)
        data_ = static_cast<const std::byte*>(mapped); // store pointer to mapped bytes
    }

    ~MappedFile() {
        if (data_ != nullptr) {
            ::munmap(const_cast<std::byte*>(data_), size_);
        }
    }

    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    MappedFile(MappedFile&& other) noexcept
        : data_{std::exchange(other.data_, nullptr)},
          size_{std::exchange(other.size_, 0)} {}

    MappedFile& operator=(MappedFile&& other) noexcept {
        if (this != &other) {
            if (data_ != nullptr) {
                ::munmap(const_cast<std::byte*>(data_), size_);
            }
            data_ = std::exchange(other.data_, nullptr);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    [[nodiscard]] std::span<const std::byte> bytes() const noexcept { return {data_, size_}; }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }

private:
    const std::byte* data_ = nullptr;
    std::size_t size_ = 0;
};

} 
