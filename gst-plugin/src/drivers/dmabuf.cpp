//
// Created by matin on 2025-07-27.
//

#include "dmabuf.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include "config.h"

/* This block of code is only accessible from C code. */
#ifdef HAVE_MMNGR
extern "C" {
#include "mmngr_buf_user_public.h"
#include "mmngr_user_public.h"
}
#endif

DMABuffer::DMABuffer(const uint32_t buf_size) : size(buf_size)
{
#ifdef HAVE_MMNGR
    MMNGR_ID id       = 0;
    int      m_dma_fd = 0;

    int ret = mmngr_alloc_in_user_ext(&idx, size, &phy_addr, &mem, MMNGR_VA_SUPPORT_CACHED, nullptr);
    if (ret < 0) {
        throw std::runtime_error("[ERROR] Can't allocate user ext in mmngr: " + std::to_string(ret));
    }

    // Write once to allocate physical memory to u-dma-buf virtual space.
    std::memset(mem, 0, size);

    ret = mmngr_export_start_in_user_ext(&id, size, phy_addr, &m_dma_fd, nullptr);
    if (ret < 0) {
        throw std::runtime_error("[ERROR] Can't export start user ext in mmngr: " + std::to_string(ret));
    }
#else
    mem = malloc(size);
#endif
}

DMABuffer::~DMABuffer()
{
#ifdef HAVE_MMNGR
    if (const int ret = mmngr_free_in_user_ext(idx); ret < 0) {
        std::cerr << "[ERROR] Can't free user ext in mmngr: " << ret << std::endl;
    }
#else
    free(mem);
#endif
}

void DMABuffer::copy(const void *src) const { memcpy(mem, src, size); }

void DMABuffer::flush() const
{
#ifdef HAVE_MMNGR
    if (const int ret = mmngr_flush(idx, 0, size); ret < 0) {
        throw std::runtime_error("[ERROR] Can't flush mmngr: " + std::to_string(ret));
    }
#endif
}
