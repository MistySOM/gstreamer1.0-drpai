#include "gst_udma_buffer_pool.h"
#include <iostream>
#include "drivers/dmabuf.h"

struct Gst_UDMA_BufferPoolClass : GstBufferPoolClass {
};

G_DEFINE_TYPE(Gst_UDMA_BufferPool, gst_udma_buffer_pool, GST_TYPE_BUFFER_POOL);

GstFlowReturn gst_udma_buffer_pool_alloc_buffer(GstBufferPool *pool, GstBuffer **buffer,
                                                GstBufferPoolAcquireParams *params)
{
    // auto *const    udma_buffer_pool = reinterpret_cast<Gst_UDMA_BufferPool *>(pool);
    constexpr auto imageLength = 640 * 480 * 3;

    std::cout << "\tUDMA Buffer allocated by other gstreamer elements." << std::endl;

    auto *const mem_ptr = DMABuffer::instance(imageLength)->get_mem();

    *buffer = gst_buffer_new_wrapped(mem_ptr, imageLength);

    return GST_FLOW_OK;
}

void gst_udma_buffer_pool_class_init(Gst_UDMA_BufferPoolClass *klass)
{
    auto *buffer_pool_class = GST_BUFFER_POOL_CLASS(klass);

    buffer_pool_class->alloc_buffer = gst_udma_buffer_pool_alloc_buffer;
}

void gst_udma_buffer_pool_init(Gst_UDMA_BufferPool *self) {}

Gst_UDMA_BufferPool *gst_udma_buffer_pool_new()
{
    return static_cast<Gst_UDMA_BufferPool *>(g_object_new(gst_udma_buffer_pool_get_type(), nullptr));
}
