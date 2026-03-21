//
// Created by matin on 2025-07-27.
//

#include "dmabuf.h"
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include "config.h"
#include "consts.h"

#ifdef HAVE_MMNGR
/* This block of code is only accessible from C code. */
extern "C" {
#include "mmngr_buf_user_public.h"
#include "mmngr_user_public.h"
}
#else
#include <fcntl.h>
#include <fstream>
#include <sys/mman.h>
#include <unistd.h>
#endif

/* Instance of DMABuffer for singleton design pattern */
static std::unique_ptr<DMABuffer> singleton_instance;

DMABuffer *DMABuffer::instance(uint32_t buf_size)
{
    if (singleton_instance == nullptr) {
        singleton_instance = std::make_unique<DMABuffer>(buf_size);
    }
    return singleton_instance.get();
}

void DMABuffer::release()
{
    if (singleton_instance != nullptr) {
        singleton_instance.reset();
    }
}


DMABuffer::DMABuffer(const uint32_t buf_size) : size(buf_size)
{
#ifdef HAVE_MMNGR
    MMNGR_ID id = 0;

    int ret = mmngr_alloc_in_user_ext(&idx, size, &phy_addr, &mem, MMNGR_VA_SUPPORT_CACHED, nullptr);
    if (ret < 0) {
        throw std::runtime_error("Can't allocate user ext in mmngr: " + std::to_string(ret));
    }

    // Write once to allocate physical memory to u-dma-buf virtual space.
    std::memset(mem, 0, size);

    ret = mmngr_export_start_in_user_ext(&id, size, phy_addr, &fd, nullptr);
    if (ret < 0) {
        throw std::runtime_error("Can't export start user ext in mmngr: " + std::to_string(ret));
    }
#else

    std::ifstream phy_addr_file("/sys/class/u-dma-buf/udmabuf0/phys_addr");
    phy_addr_file >> std::hex >> phy_addr;
    phy_addr_file.close();

    fd = open("/dev/udmabuf0", O_RDWR);
    if (fd == -1) {
        throw std::runtime_error("UDMA open failed: " + std::string(strerror(errno)));
    }

    mem = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        throw std::runtime_error("UDMA mmap failed: " + std::string(strerror(errno)));
    }

    // Write once to allocate physical memory to u-dma-buf virtual space.
    std::memset(mem, 0, size);
#endif
}

DMABuffer::~DMABuffer()
{
#ifdef HAVE_MMNGR
    if (const int ret = mmngr_free_in_user_ext(idx); ret < 0) {
        std::cerr << ERROR << "Can't free user ext in mmngr: " << ret << std::endl;
    }
#else
    if (const int ret = munmap(mem, size); ret < 0) {
        std::cerr << ERROR << "Can't unmap UDMA memory: " << ret << std::endl;
    }
    if (fd > 0) {
        if (const int ret = close(fd); ret < 0) {
            std::cerr << ERROR << "Can't close UDMA file descriptor: " << ret << std::endl;
        }
    }
#endif
}

void DMABuffer::copy(const void *src) const { memcpy(mem, src, size); }

void DMABuffer::flush() const
{
#ifdef HAVE_MMNGR
    if (const int ret = mmngr_flush(idx, 0, size); ret < 0) {
        throw std::runtime_error("Can't flush mmngr: " + std::to_string(ret));
    }
#endif
}
