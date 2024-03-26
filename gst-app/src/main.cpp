/* Copyright (C) 2006 Tim-Philipp Müller <tim centricular net>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstdarg>

#define DELAY_VALUE 100000

static GstElement *pipeline;

static gboolean
message_cb (GstBus * bus, GstMessage * message, gpointer user_data)
{
    switch (GST_MESSAGE_TYPE (message)) {
        case GST_MESSAGE_ERROR:{
            GError *err = NULL;
            gchar *name, *debug = NULL;

            name = gst_object_get_path_string (message->src);
            gst_message_parse_error (message, &err, &debug);

            g_printerr ("ERROR: from element %s: %s\n", name, err->message);
            if (debug != NULL)
                g_printerr ("Additional debug info:\n%s\n", debug);

            g_error_free (err);
            g_free (debug);
            g_free (name);
            return FALSE;
        }
        case GST_MESSAGE_WARNING:{
            GError *err = NULL;
            gchar *name, *debug = NULL;

            name = gst_object_get_path_string (message->src);
            gst_message_parse_warning (message, &err, &debug);

            g_printerr ("WARNING: from element %s: %s\n", name, err->message);
            if (debug != NULL)
                g_printerr ("Additional debug info:\n%s\n", debug);

            g_error_free (err);
            g_free (debug);
            g_free (name);
            break;
        }
        case GST_MESSAGE_EOS:{
            g_print ("Got EOS\n");
            gst_element_set_state (pipeline, GST_STATE_NULL);
            return FALSE;
        }
        default:
            break;
    }

    return TRUE;
}

int sigintHandler(int unused) {
    g_print("You ctrl-c-ed! Sending EoS");
    gst_element_send_event(pipeline, gst_event_new_eos());
    return 0;
}

template <class ... Args>
GstElement* element(const gchar* name, Args ... args) {
    GstElement* e = gst_element_factory_make(name, name);
    if(!e)
        throw std::runtime_error(std::string("Failed to create element: ") + name);
    if (std::get<0>(std::tie(args...)))
        g_object_set(e, args...);
    return e;
}

template <class ... Args>
GstElement* create_pipeline(Args ... args) {
    GstElement* p = gst_pipeline_new(nullptr);
    if(!p)
        throw std::runtime_error("Failed to create pipeline");
    gst_bin_add_many(GST_BIN(p), args...);
    if (!gst_element_link_many(args...))
        throw std::runtime_error("Failed to link elements");
    return p;
}

int main (int argc, char *argv[]) {
    signal(SIGINT, (__sighandler_t) sigintHandler);
    gst_init(&argc, &argv);

    pipeline = create_pipeline(
        element("v4l2src", "device", "/dev/video0", nullptr),
        element("drpai", "model","yolov3",
                "show-fps",FALSE,"log-detects",FALSE,
                "smooth-video-rate",30,
                "filter-class","cup:1400fa", nullptr),
        element("vspmfilter","dmabuf-use",TRUE, nullptr),
        element("omxh264enc","control-rate",2,"target-bitrate",10485760, "interval_intraframes",14,"periodicty-idr",2, nullptr),
        element("rtph264pay","config-interval",-1, nullptr),
        element("udpsink", "host","192.168.99.2","port",51372, nullptr),
        nullptr
    );

    if (gst_element_set_state (pipeline,
                               GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (pipeline);
        return -1;
    }

    GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE(pipeline));
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_print("Starting loop. Send SIGINT to exit.\n");
    while(true) {
        GstMessage* msg = gst_bus_pop (bus);
        /* Note that because input timeout is GST_CLOCK_TIME_NONE,
           the gst_bus_timed_pop_filtered() function will block forever until a
           matching message was posted on the bus (GST_MESSAGE_ERROR or
           GST_MESSAGE_EOS). */
        if (msg != nullptr) {
            gchar *debug_info;
            gboolean r = message_cb(bus, msg, debug_info);
            gst_message_unref (msg);

            if(r == FALSE)
                break;
        }
    }

    gst_object_unref (bus);
    g_print ("Returned, stopping playback...\n");
    gst_element_set_state (pipeline, GST_STATE_NULL);
    g_print ("Freeing pipeline...\n");
    gst_object_unref (GST_OBJECT (pipeline));
    g_print ("Completed. Goodbye!\n");
    return 0;
}
