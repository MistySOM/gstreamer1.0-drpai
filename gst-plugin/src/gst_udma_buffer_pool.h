#pragma once

#include <gst/gstbufferpool.h>

struct Gst_UDMA_BufferPool : public GstBufferPool {
};

Gst_UDMA_BufferPool *gst_udma_buffer_pool_new();
