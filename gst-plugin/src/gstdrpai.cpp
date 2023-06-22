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
#  include <config.h>
#endif

#include <gst/gst.h>
#include "gstdrpai.h"
#include "drpai.h"

GST_DEBUG_CATEGORY_STATIC (gst_drpai_debug);
#define GST_CAT_DEFAULT gst_drpai_debug

/* Filter signals and args */
enum {
    /* FILL ME */
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_MULTITHREAD,
    PROP_MODEL,
    PROP_SHOW_FPS,
    PROP_LOG_DETECTS,
    PROP_STOP_ERROR,

    PROP_MAX_VIDEO_RATE,
    PROP_MAX_DRPAI_RATE,
    PROP_SMOOTH_VIDEO_RATE,
    PROP_SMOOTH_DRPAI_RATE,
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
auto pad_caps = "video/x-raw, width = (int) 640, height = (int) 480, format = (string) BGR";
static GstStaticPadTemplate sink_factory =
        GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(pad_caps));
static GstStaticPadTemplate src_factory =
        GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(pad_caps));

#define gst_drpai_parent_class parent_class

G_DEFINE_TYPE (GstDRPAI, gst_drpai, GST_TYPE_ELEMENT);

static void gst_drpai_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static void gst_drpai_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gboolean gst_drpai_sink_event(GstPad *pad, GstObject *parent, GstEvent *event);

static GstFlowReturn gst_drpai_chain(GstPad *pad, GstObject *parent, GstBuffer *buf);

static GstStateChangeReturn gst_drpai_change_state (GstElement * element, GstStateChange transition);

/* GObject vmethod implementations */

/* initialize the plugin's class */
static void
gst_drpai_class_init(GstDRPAIClass *klass) {
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = (GObjectClass *) klass;
    gstelement_class = (GstElementClass *) klass;

    gobject_class->set_property = gst_drpai_set_property;
    gobject_class->get_property = gst_drpai_get_property;

    gstelement_class->change_state = gst_drpai_change_state;

    g_object_class_install_property(gobject_class, PROP_MULTITHREAD,
        g_param_spec_boolean("multithread", "MultiThread",
                             "Use a separate thread for object detection.",
                             TRUE, G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_MODEL,
        g_param_spec_string("model", "Model",
                             "The name of the pretrained model and the directory prefix.",
                             nullptr, G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_LOG_DETECTS,
        g_param_spec_boolean("log_detects", "Log Detects",
                             "Print detected objects in standard output.",
                             FALSE, G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_SHOW_FPS,
        g_param_spec_boolean("show_fps", "Show Frame Rates",
                             "Render frame rates of video and DRPAI at the corner of the video.",
                             FALSE, G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_STOP_ERROR,
        g_param_spec_boolean("stop_error", "Stop On Errors",
                             "Stop the gstreamer if kernel modules fail to open.",
                             TRUE, G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_MAX_VIDEO_RATE,
        g_param_spec_float("max_video_rate", "Max Video Framerate",
                           "Force maximum video frame rate using thread sleeps.",
                           0.001f, 120.f, 120.f, G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_MAX_DRPAI_RATE,
        g_param_spec_float("max_drpai_rate", "Max DRPAI Framerate",
                           "Force maximum DRPAI frame rate using thread sleeps.",
                            0.0f, 120.f, 120.f, G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_SMOOTH_VIDEO_RATE,
        g_param_spec_uint("smooth_video_rate", "Smooth Video Framerate",
                             "Number of last video frame rates to average for a more smooth value.",
                             1, 1000, 1, G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_SMOOTH_DRPAI_RATE,
        g_param_spec_uint("smooth_drpai_rate", "Smooth DRPAI Framerate",
                             "Number of last DRPAI frame rates to average for a more smooth value.",
                             1, 1000, 1, G_PARAM_READWRITE));

    gst_element_class_set_details_simple(gstelement_class,
                                         "DRP-AI",
                                         "DRP-AI",
                                         "DRP-AI Element", "Matin Lotfaliei matin.lotfali@mistywest.com");

    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&src_factory));
    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad callback functions
 * initialize instance structure
 */
static void
gst_drpai_init(GstDRPAI *obj) {
    obj->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
    gst_pad_set_event_function (obj->sinkpad,
                                GST_DEBUG_FUNCPTR(gst_drpai_sink_event));
    gst_pad_set_chain_function (obj->sinkpad,
                                GST_DEBUG_FUNCPTR(gst_drpai_chain));
    GST_PAD_SET_PROXY_CAPS (obj->sinkpad);
    gst_element_add_pad(GST_ELEMENT (obj), obj->sinkpad);

    obj->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
    GST_PAD_SET_PROXY_CAPS (obj->srcpad);
    gst_element_add_pad(GST_ELEMENT (obj), obj->srcpad);

    obj->drpai = new DRPAI();
    obj->stop_error = TRUE;
}

static GstStateChangeReturn
gst_drpai_change_state (GstElement * element, GstStateChange transition) {
    GstStateChangeReturn state_change_ret = GST_STATE_CHANGE_SUCCESS;
    auto *obj = (GstDRPAI*) &element->object;

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            /* open the device */
            if ( obj->drpai->open_resources() == -1 )
                if (obj->stop_error)
                    return GST_STATE_CHANGE_FAILURE;
            break;
        default:
            break;
    }

    state_change_ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

    int ret;
    switch (transition) {
        case GST_STATE_CHANGE_READY_TO_NULL:
            /* close the device */
            ret = obj->drpai->release_resources();
            delete obj->drpai;
            if (ret == -1)
                return GST_STATE_CHANGE_FAILURE;
            break;
        default:
            break;
    }

    return state_change_ret;
}

static void
gst_drpai_set_property(GObject *object, guint prop_id,
                              const GValue *value, GParamSpec *pspec) {
    GstDRPAI *obj = GST_PLUGIN_DRPAI(object);

    switch (prop_id) {
        case PROP_MULTITHREAD:
            obj->drpai->multithread = g_value_get_boolean(value);
            break;
        case PROP_MODEL:
            obj->drpai->model_prefix = g_value_get_string(value);
            break;
        case PROP_LOG_DETECTS:
            obj->drpai->log_detects = g_value_get_boolean(value);
            break;
        case PROP_SHOW_FPS:
            obj->drpai->show_fps = g_value_get_boolean(value);
            break;
        case PROP_STOP_ERROR:
            obj->stop_error = g_value_get_boolean(value);
            break;
        case PROP_MAX_VIDEO_RATE:
            obj->drpai->video_rate.max_rate = g_value_get_float(value);
            break;
        case PROP_MAX_DRPAI_RATE:
            obj->drpai->drpai_rate.max_rate = g_value_get_float(value);
            break;
        case PROP_SMOOTH_VIDEO_RATE:
            obj->drpai->video_rate.smooth_rate = g_value_get_uint(value);
            break;
        case PROP_SMOOTH_DRPAI_RATE:
            obj->drpai->drpai_rate.smooth_rate = g_value_get_uint(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gst_drpai_get_property(GObject *object, guint prop_id,
                              GValue *value, GParamSpec *pspec) {
    GstDRPAI *obj = GST_PLUGIN_DRPAI(object);

    switch (prop_id) {
        case PROP_MULTITHREAD:
            g_value_set_boolean(value, obj->drpai->multithread);
            break;
        case PROP_MODEL:
            g_value_set_string(value, obj->drpai->model_prefix.c_str());
            break;
        case PROP_LOG_DETECTS:
            g_value_set_boolean(value, obj->drpai->log_detects);
            break;
        case PROP_SHOW_FPS:
            g_value_set_boolean(value, obj->drpai->show_fps);
            break;
        case PROP_STOP_ERROR:
            g_value_set_boolean(value, obj->stop_error);
            break;
        case PROP_MAX_VIDEO_RATE:
            g_value_set_float(value, obj->drpai->video_rate.max_rate);
            break;
        case PROP_MAX_DRPAI_RATE:
            g_value_set_float(value, obj->drpai->drpai_rate.max_rate);
            break;
        case PROP_SMOOTH_VIDEO_RATE:
            g_value_set_uint(value, obj->drpai->video_rate.smooth_rate);
            break;
        case PROP_SMOOTH_DRPAI_RATE:
            g_value_set_uint(value, obj->drpai->drpai_rate.smooth_rate);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_drpai_sink_event(GstPad *pad, GstObject *parent,
                            GstEvent *event) {
    GstDRPAI *obj;
    gboolean ret;

    obj = GST_PLUGIN_DRPAI(parent);

    GST_LOG_OBJECT (obj, "Received %s event: %" GST_PTR_FORMAT,
                    GST_EVENT_TYPE_NAME(event), event);

    switch (GST_EVENT_TYPE (event)) {
        case GST_EVENT_CAPS: {
            GstCaps *caps;

            gst_event_parse_caps(event, &caps);
            /* do something with the caps */

            /* and forward */
            ret = gst_pad_event_default(pad, parent, event);
            break;
        }
        default:
            ret = gst_pad_event_default(pad, parent, event);
            break;
    }
    return ret;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_drpai_chain(GstPad *pad, GstObject *parent, GstBuffer *buf) {
    GstDRPAI *obj;

    obj = GST_PLUGIN_DRPAI(parent);

    GstMapInfo info;
    gst_buffer_map(buf, &info, GST_MAP_READWRITE);

    if (obj->drpai->process_image(info.data) == -1) {
        if(obj->stop_error) {
            gst_buffer_unref (buf);
            return GST_FLOW_ERROR;
        }
    }

    gst_buffer_unmap(buf, &info);

    /* just push out the incoming buffer without touching it */
    return gst_pad_push(obj->srcpad, buf);
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
