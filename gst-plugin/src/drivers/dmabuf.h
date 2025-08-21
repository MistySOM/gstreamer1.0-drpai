//
// Created by matin on 2025-07-27.
//

#pragma once

#include <cstdint>

class DMABuffer
{
public:
    explicit DMABuffer(uint32_t buf_size);
    ~DMABuffer();

    // Delete copy constructor and copy assignment operator
    DMABuffer(const DMABuffer &)                     = delete;
    DMABuffer &operator=(const DMABuffer &)          = delete;
    DMABuffer(DMABuffer &&other) noexcept            = delete;
    DMABuffer &operator=(DMABuffer &&other) noexcept = delete;

    void copy(const void *src) const;
    void flush() const;

    [[nodiscard]] constexpr uint8_t *get_mem() const { return static_cast<uint8_t *>(mem); }
    [[nodiscard]] constexpr uint32_t get_physical_address() const { return phy_addr; }

private:
    /* The index of the buffer. */
    int idx = 0;
    /* The size of the buffer in bytes. */
    const uint32_t size;
    /* The physical address of DMA buffer. */
    uint32_t phy_addr = 0;
    /* The pointer to the memory for the buffer. */
    void *mem = nullptr;
};
