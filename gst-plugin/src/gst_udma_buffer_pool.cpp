//
// Created by matin on 20/06/24.
//

#include "gst_udma_buffer_pool.h"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

uint32_t Gst_UDMA_BufferPool::get_physical_address() {
    char addr[1024];
    errno = 0;
    const auto fd = open("/sys/class/u-dma-buf/udmabuf0/phys_addr", O_RDONLY);
    if (0 > fd)
        throw std::runtime_error("[ERROR] Failed to open udmabuf0/phys_addr : errno="  + std::string(std::strerror(errno)));
    if ( read(fd, addr, 1024) < 0 )
    {
        close(fd);
        throw std::runtime_error("[ERROR] Failed to read udmabuf0/phys_addr :  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));
    }
    uint64_t udmabuf_address = std::strtoul(addr, nullptr, 16);
    close(fd);
    /* Filter the bit higher than 32 bit */
    udmabuf_address &=0xFFFFFFFF;
    return udmabuf_address;
}

struct Gst_UDMA_BufferPoolClass {
    GstBufferPoolClass parent_class;
};

G_DEFINE_TYPE(Gst_UDMA_BufferPool, gst_udma_buffer_pool, GST_TYPE_BUFFER_POOL);


GstFlowReturn
gst_udma_buffer_pool_alloc_buffer (GstBufferPool *pool, GstBuffer **buffer, GstBufferPoolAcquireParams *params) {
    std::cout << "UDMA Buffer allocated by other gstreamer elements." << std::endl;
    constexpr uint32_t MAPSIZE = 0x1000;
    constexpr uint32_t MAPMASK = 0xFFFFF000;

    const auto udma_buffer_pool = reinterpret_cast<Gst_UDMA_BufferPool*>(pool);
    constexpr auto imageLength = 640*480*3;
    constexpr auto offset = (imageLength + MAPSIZE - 1) & MAPMASK;
    const auto mem_ptr = mmap(nullptr, imageLength, PROT_READ | PROT_WRITE, MAP_SHARED,
        udma_buffer_pool->udmabuf_fd,
        offset * udma_buffer_pool->pool_index);
    ++udma_buffer_pool->pool_index;

    if (mem_ptr == MAP_FAILED) {
        std::cerr << "[ERROR] UDMA mmap failed: " << strerror(errno) << std::endl;
        return GST_FLOW_ERROR;
    }

    // Write once to allocate physical memory to u-dma-buf virtual space.
    const auto word_ptr = static_cast<uint8_t*>(mem_ptr);
    std::fill_n(word_ptr, imageLength, 0);

    *buffer = gst_buffer_new_wrapped(mem_ptr, imageLength);

    return GST_FLOW_OK;
}

void gst_udma_buffer_pool_free_buffer(GstBufferPool *pool, GstBuffer *buffer) {
    const auto memory = gst_buffer_get_memory(buffer, 0);

    // Map the memory to get access to the data
    GstMapInfo info;
    if (gst_memory_map(memory, &info, GST_MAP_READWRITE)) {
        // Get the data pointer and size from the map info
        void *data_ptr = info.data;
        const gsize size = info.size;

        // Unmap the memory before unmapping with munmap
        gst_memory_unmap(memory, &info);

        // Unmap the memory
        munmap(data_ptr, size);
    }
}

void gst_udma_buffer_pool_finalize(GObject* obj) {
    std::cout << "UDMA Buffer finalizing." << std::endl;
    G_OBJECT_CLASS(gst_udma_buffer_pool_parent_class)->finalize(obj);

    auto self = reinterpret_cast<Gst_UDMA_BufferPool*>(obj);
    if (self->udmabuf_fd > 0) {
        close(self->udmabuf_fd);
        self->udmabuf_fd = 0;
    }
}
void Gst_UDMA_BufferPool::finalize() {
    gst_udma_buffer_pool_finalize(reinterpret_cast<GObject*>(this));
}

void gst_udma_buffer_pool_class_init(Gst_UDMA_BufferPoolClass *klass) {
    auto *buffer_pool_class = GST_BUFFER_POOL_CLASS(klass);
    auto *gobject_class = G_OBJECT_CLASS(klass);

    buffer_pool_class->alloc_buffer = gst_udma_buffer_pool_alloc_buffer;
    buffer_pool_class->free_buffer = gst_udma_buffer_pool_free_buffer;
    gobject_class->finalize = gst_udma_buffer_pool_finalize;
}

void gst_udma_buffer_pool_init(Gst_UDMA_BufferPool *self) {
    std::cout << "UDMA Buffer initializing." << std::endl;
    self->pool_index = 0;
    self->udmabuf_fd = open("/dev/udmabuf0", O_RDWR);
    if (self->udmabuf_fd < 0) {
        throw std::runtime_error("[Error] Failed to open /dev/udmabuf0");
    }
}

Gst_UDMA_BufferPool* gst_udma_buffer_pool_new() {
    return static_cast<Gst_UDMA_BufferPool *>(g_object_new(gst_udma_buffer_pool_get_type(), nullptr));
}