/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) YEAR AUTHOR_NAME AUTHOR_EMAIL
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-plugin
 *
 * DRP-AI Plugin
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! drpai ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gstdrpai.h"
#include "gst_udma_buffer_pool.h"
#include "properties.h"
#include "drpai_controller.h"
#include <gst/gst.h>
#include <iostream>

static gboolean gst_drpai_sink_query(GstPad *pad, GstObject *parent, GstQuery *query) {
    std::cout << "DRP-AI received query: " << GST_QUERY_TYPE_NAME(query) << std::endl;
    const auto obj = GST_PLUGIN_DRPAI(parent);

    switch (query->type) {
        case GST_QUERY_ALLOCATION:
            if (obj->drpai_controller->share_udma_buffer) {
                const auto pool = reinterpret_cast<GstBufferPool*>(obj->udma_buffer_pool);
                gst_query_add_allocation_pool(query, pool, 0, 0, 0);
                std::cout << "UDMA buffer allocation pool provided." << std::endl;
                return TRUE;
            }
            break;
        default:
            break;
    }

    return FALSE;
}

static GstStateChangeReturn
gst_drpai_change_state (GstElement * element, const GstStateChange transition) {
    auto *obj = reinterpret_cast<GstDRPAI*>(&element->object);

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            try {
                obj->udma_buffer_pool = gst_udma_buffer_pool_new();

                /* Obtain udmabuf memory area starting address */
                const auto udmabuf_address = Gst_UDMA_BufferPool::get_physical_address();

                /* open the device */
                obj->drpai_controller->open_resources(obj->udma_buffer_pool->udmabuf_fd, udmabuf_address);
            }
            catch (std::runtime_error &e) {
                std::cerr << std::endl << e.what() << std::endl << std::endl;
                if (obj->stop_error)
                    return GST_STATE_CHANGE_FAILURE;
            }
            break;
        default:
            break;
    }

    auto state_change_ret = GST_ELEMENT_CLASS (parent_class)->change_state(element, transition);

    switch (transition) {
        case GST_STATE_CHANGE_READY_TO_NULL:
            try {
                /* close the device */
                obj->drpai_controller->release_resources();
            }
            catch (std::runtime_error &e) {
                std::cerr << std::endl << e.what() << std::endl << std::endl;
                state_change_ret = GST_STATE_CHANGE_FAILURE;
            }
            delete obj->drpai_controller;
            break;
        default:
            break;
    }
    return state_change_ret;
}
static void
gst_drpai_set_property(GObject *object, const guint prop_id,
                              const GValue *value, GParamSpec *pspec) {
    GstDRPAI *obj = GST_PLUGIN_DRPAI(object);

    try {
        switch (prop_id) {
            case PROP_STOP_ERROR:
                obj->stop_error = g_value_get_boolean(value);
                break;
            default:
                obj->drpai_controller->set_property(static_cast<GstDRPAI_Properties>(prop_id), value);
                break;
        }
    } catch (std::runtime_error& e) {
        std::cerr << std::endl << e.what() << std::endl << std::endl;
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        throw e;
    } catch (std::exception& e) {
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        throw e;
    }
}

static void
gst_drpai_get_property(GObject *object, const guint prop_id,
                              GValue *value, GParamSpec *pspec) {
    const GstDRPAI *obj = GST_PLUGIN_DRPAI(object);

    try {
        switch (prop_id) {
            case PROP_STOP_ERROR:
                g_value_set_boolean(value, obj->stop_error);
                break;
            default:
                obj->drpai_controller->get_property(static_cast<GstDRPAI_Properties>(prop_id), value);
                break;
        }
    } catch (std::exception&) {
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_drpai_sink_event(GstPad *pad, GstObject *parent, GstEvent *event) {

    // const auto obj = GST_PLUGIN_DRPAI(parent);
    // std::cout << "DRP-AI received event: "<< GST_EVENT_TYPE_NAME(event) << std::endl;

    switch (GST_EVENT_TYPE (event)) {
        case GST_EVENT_CAPS: {
            GstCaps *caps;

            gst_event_parse_caps(event, &caps);
            /* do something with the caps */

            // std::cout << "\tCaps: " << gst_caps_to_string(caps) << std::endl;

            /* and forward */
            return gst_pad_event_default(pad, parent, event);
        }
        default:
            return gst_pad_event_default(pad, parent, event);
    }
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_drpai_chain(GstPad *pad, GstObject *parent, GstBuffer *buf) {
    
    const auto obj = GST_PLUGIN_DRPAI(parent);

    GstMapInfo info;
    gst_buffer_map(buf, &info, GST_MAP_READWRITE);

    try {
        obj->drpai_controller->process_image(info.data);
    }
    catch (const std::exception& e) {
        std::cerr << std::endl << e.what() << std::endl << std::endl;
        if(obj->stop_error) {
            gst_buffer_unref (buf);
            return GST_FLOW_ERROR;
        }
    }

    gst_buffer_unmap(buf, &info);

    /* just push out the incoming buffer without touching it */
    return gst_pad_push(obj->srcpad, buf);
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad callback functions
 * initialize instance structure
 */
static void
gst_drpai_init(GstDRPAI* self) {
    self->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
    gst_pad_set_event_function (self->sinkpad, GST_DEBUG_FUNCPTR(gst_drpai_sink_event));
    gst_pad_set_query_function(self->sinkpad, GST_DEBUG_FUNCPTR(gst_drpai_sink_query));
    gst_pad_set_chain_function (self->sinkpad, GST_DEBUG_FUNCPTR(gst_drpai_chain));
    GST_PAD_SET_PROXY_CAPS (self->sinkpad);
    gst_element_add_pad(GST_ELEMENT (self), self->sinkpad);

    self->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
    GST_PAD_SET_PROXY_CAPS (self->srcpad);
    gst_element_add_pad(GST_ELEMENT (self), self->srcpad);

    self->drpai_controller = new DRPAI_Controller();
    self->stop_error = TRUE;
}

/* initialize the plugin's class */
static void
gst_drpai_class_init(GstDRPAIClass *klass) {
    const auto gobject_class = reinterpret_cast<GObjectClass *>(klass);
    const auto gstelement_class = reinterpret_cast<GstElementClass *>(klass);

    gobject_class->set_property = gst_drpai_set_property;
    gobject_class->get_property = gst_drpai_get_property;

    gstelement_class->change_state = gst_drpai_change_state;

    std::map<GstDRPAI_Properties, _GParamSpec*> params;
    params.emplace(PROP_STOP_ERROR, g_param_spec_boolean("stop_error", "Stop On Errors",
                                                      "Stop the gstreamer if kernel modules fail to open.",
                                                      TRUE, G_PARAM_READWRITE));
    DRPAI_Controller::install_properties(params);

    for (auto& [prop_id, spec]: params)
        g_object_class_install_property(gobject_class, prop_id, spec);

    gst_element_class_set_details_simple(gstelement_class,
                                         "DRP-AI",
                                         "DRP-AI",
                                         "DRP-AI Element", "Matin Lotfaliei matin.lotfali@mistywest.com");

    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&src_factory));
    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&sink_factory));
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
plugin_init(GstPlugin *plugin) {
    /* debug category for filtering log messages */
    GST_DEBUG_CATEGORY_INIT (gst_drpai_debug, "drpai", 0, "DRP-AI plugin");
    return gst_element_register (plugin, "drpai", GST_RANK_NONE, GST_TYPE_PLUGIN_DRPAI);
}

/* PACKAGE: this is usually set by meson depending on some _INIT macro
 * in meson.build and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use meson to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstplugin"
#endif

/* gstreamer looks for this structure to register plugins */
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
                   GST_VERSION_MINOR,
                   drpai,
                   "DRP-AI Plug-in",
                   plugin_init,
                   PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
