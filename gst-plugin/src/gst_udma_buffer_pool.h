//
// Created by matin on 11/06/24.
//

#ifndef GST_UDMA_BUFFER_POOL_H
#define GST_UDMA_BUFFER_POOL_H

#include <gst/gstbufferpool.h>
#include <cstdint>

struct Gst_UDMA_BufferPool {
    GstBufferPool parent;
    int udmabuf_fd;
    uint8_t pool_index;

    void finalize();
    static uint32_t get_physical_address();
};

Gst_UDMA_BufferPool* gst_udma_buffer_pool_new();

#endif //GST_UDMA_BUFFER_POOL_H
