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
#include "messages.h"
#ifdef ENABLE_SPLIT
#include "split_thread.h"
#endif

GstElement *pipeline;

int signalHandler(int signal) {
    g_print("\n\nSignal %s received! Exiting...\n\n", strsignal(signal));
    gst_element_send_event(pipeline, gst_event_new_eos());
    return 0;
}

int main (int argc, char *argv[]) {
    if (argc < 2)
        g_error("Need a pipeline launch argument, starting with src and ending with sink.");

    signal(SIGINT, (__sighandler_t) signalHandler);
    signal(SIGTERM, (__sighandler_t) signalHandler);

    std::string arg = argv[1];
    for (int i = 2; i<argc; i++)
        arg += std::string(" ") + argv[i];

    gst_init(&argc, &argv);

    g_print ("Loading pipeline...\n");
    GError *error = nullptr;
    pipeline = gst_parse_launch(arg.c_str(), &error);
    if (error)
        g_error ("Parse error: %s\n", error->message);

#ifdef ENABLE_SPLIT
    auto splitmuxsink = gst_bin_get_by_name( GST_BIN( pipeline ), "my_split_mux" );
    split_thread thread (splitmuxsink);
#endif

    GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE(pipeline));
    if (!bus)
        g_error ("Unable to get the bus of the pipeline\n");

    g_print ("Starting pipeline...\n");
    auto state_change_return = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (state_change_return == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (bus);
        gst_object_unref (pipeline);
        return -1;
    }

    g_print("Starting loop. Send SIGINT or SIGTERM to exit.\n");
    while(true) {
        GstMessage* msg = gst_bus_pop (bus);
        /* Note that because input timeout is GST_CLOCK_TIME_NONE,
           the gst_bus_timed_pop_filtered() function will block forever until a
           matching message was posted on the bus (GST_MESSAGE_ERROR or
           GST_MESSAGE_EOS). */
        if (msg != nullptr) {
            gboolean r = message_cb(pipeline, msg);
            gst_message_unref (msg);

            if(r == FALSE)
                break;
        }
    }

#ifdef ENABLE_SPLIT
    thread.stop();
#endif

    g_print ("Freeing pipeline...");
    state_change_return = gst_element_set_state (pipeline, GST_STATE_NULL);
    g_print("%s\n", gst_element_state_change_return_get_name(state_change_return));
    gst_object_unref (bus);
    gst_object_unref (GST_OBJECT (pipeline));
    return 0;
}
