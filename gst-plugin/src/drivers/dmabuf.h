//
// Created by matin on 2025-07-27.
//

#ifndef GSTREAMER1_0_DRPAI_DMABUF_H
#define GSTREAMER1_0_DRPAI_DMABUF_H

#include<cstdint>

class DMABuffer {
public:
    explicit DMABuffer(uint32_t buf_size);
    ~DMABuffer();

    void copy(const void* src) const;
    void flush() const;

    uint8_t* get_mem() const { return static_cast<uint8_t*>(mem); }
    uint32_t get_physical_address() const { return phy_addr; }

private:
    /* The index of the buffer. */
    int idx = 0;
    /* The size of the buffer in bytes. */
    const uint32_t size;
    /* The physical address of DMA buffer. */
    uint32_t phy_addr;
    /* The pointer to the memory for the buffer. */
    void *mem = nullptr;
};


#endif //GSTREAMER1_0_DRPAI_DMABUF_H