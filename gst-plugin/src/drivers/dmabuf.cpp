//
// Created by matin on 2025-07-27.
//

#include "dmabuf.h"
#include <string>
#include <cstring>
#include <stdexcept>
#include <iostream>

/* This block of code is only accessible from C code. */
extern "C" {
#include "mmngr_user_public.h"
#include "mmngr_buf_user_public.h"
}

DMABuffer::DMABuffer(const uint32_t buf_size):
    size(buf_size)
{
    MMNGR_ID id;
    int m_dma_fd;
    uint32_t phard_addr;

    int ret = mmngr_alloc_in_user_ext(&idx, size, &phard_addr, &mem, MMNGR_VA_SUPPORT_CACHED, nullptr);
    if (ret < 0) {
        throw std::runtime_error("[ERROR] Can't allocate user ext in mmngr: " + std::to_string(ret));
    }

    // Write once to allocate physical memory to u-dma-buf virtual space.
    std::memset(mem, 0, size);

    ret = mmngr_export_start_in_user_ext(&id, size, phard_addr, &m_dma_fd, nullptr);
    if (ret < 0) {
        throw std::runtime_error("[ERROR] Can't export start user ext in mmngr: " + std::to_string(ret));
    }
}

DMABuffer::~DMABuffer() {
    if (const int ret = mmngr_free_in_user_ext(idx); ret < 0) {
        std::cerr << "[ERROR] Can't free user ext in mmngr: " << ret << std::endl;
    }
}

void DMABuffer::copy(const void *src) const {
    memcpy( mem, src, size);
}

void DMABuffer::flush() const  {
    if (const int ret = mmngr_flush(idx, 0, size); ret < 0) {
        throw std::runtime_error("[ERROR] Can't flush mmngr: " + std::to_string(ret));
    }
}


