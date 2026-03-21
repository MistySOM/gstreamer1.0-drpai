#include "gst_udma_buffer_pool.h"
#include <iostream>
#include "drivers/dmabuf.h"

struct Gst_UDMA_BufferPoolClass : GstBufferPoolClass {
};

G_DEFINE_TYPE(Gst_UDMA_BufferPool, gst_udma_buffer_pool, GST_TYPE_BUFFER_POOL);

GstFlowReturn gst_udma_buffer_pool_alloc_buffer(GstBufferPool *pool, GstBuffer **buffer,
                                                GstBufferPoolAcquireParams *params)
{
    /* Buffer size comes from the pool configuration set during caps negotiation. */
    GstStructure *config = gst_buffer_pool_get_config(pool);
    guint         size   = 0;

    gst_buffer_pool_config_get_params(config, nullptr, &size, nullptr, nullptr);
    gst_structure_free(config);

    if (size == 0) {
        std::cerr << "\tUDMA Buffer pool: buffer size not configured, using default 640x480 BGR." << std::endl;
        size = 640 * 480 * 3;
    }

    std::cout << "\tUDMA Buffer allocated by other gstreamer elements." << std::endl;

    auto *const mem_ptr = DMABuffer::instance(size)->get_mem();
    /* NULL destroy notify because the DMA buffer memory is not heap-allocated
     * and must not be freed by GStreamer. */
    *buffer = gst_buffer_new_wrapped_full(static_cast<GstMemoryFlags>(0), mem_ptr, size, 0, size, nullptr, nullptr);

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
