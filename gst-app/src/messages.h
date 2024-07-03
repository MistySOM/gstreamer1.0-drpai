//
// Created by matin on 11/04/24.
//

#ifndef GSTREAMER1_0_DRPAI_MESSAGES_H
#define GSTREAMER1_0_DRPAI_MESSAGES_H

gboolean message_cb(GstElement* pipeline, GstMessage *message)
{
    switch (message->type) {
        case GST_MESSAGE_ERROR:{
            GError *err = nullptr;
            gchar *name, *debug = nullptr;

            name = gst_object_get_path_string (message->src);
            gst_message_parse_error (message, &err, &debug);

            g_printerr ("ERROR: from element %s: %s\n", name, err->message);
            if (debug != nullptr)
                g_printerr ("Additional debug info:\n%s\n", debug);

            g_error_free (err);
            g_free (debug);
            g_free (name);
            break;
        }
        case GST_MESSAGE_WARNING:{
            GError *err = nullptr;
            gchar *name, *debug = nullptr;

            name = gst_object_get_path_string (message->src);
            gst_message_parse_warning (message, &err, &debug);

            g_printerr ("WARNING: from element %s: %s\n", name, err->message);
            if (debug != nullptr)
                g_printerr ("Additional debug info:\n%s\n", debug);

            g_error_free (err);
            g_free (debug);
            g_free (name);
            break;
        }
        case GST_MESSAGE_INFO: {
            GError *err = nullptr;
            gchar *name, *debug = nullptr;

            name = gst_object_get_path_string (message->src);
            gst_message_parse_info(message, &err, &debug);

            g_printerr ("INFO: from element %s: %s\n", name, err->message);
            if (debug != nullptr)
                g_printerr ("Additional debug info:\n%s\n", debug);

            g_error_free (err);
            g_free (debug);
            g_free (name);
            break;
        }
        case GST_MESSAGE_STATE_CHANGED: {
            if (message->src == reinterpret_cast<GstObject*>(pipeline)) {
                GstState oldState, newState, pendingState;
                gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);
                const gchar* name = gst_element_state_get_name(newState);
                g_print ("State changed to %s.\n", name);
            }
            break;
        }
        case GST_MESSAGE_EOS:{
            g_print ("Got EOS. Stopping the playback...\n");
            auto r = gst_element_set_state (pipeline, GST_STATE_NULL);
            if (r != GST_STATE_CHANGE_SUCCESS)
                g_print("\t%s\n", gst_element_state_change_return_get_name(r));
            return FALSE;
        }
        case GST_MESSAGE_LATENCY: {
            g_print ("Latency changed by element %s.\n", gst_element_get_name(message->src));
            break;
        }
        case GST_MESSAGE_ASYNC_DONE: {
            g_print ("Async state change is done.\n");
            break;
        }
        case GST_MESSAGE_ELEMENT: {
            g_print ("Message from element %s: %s\n",
                     gst_element_get_name(message->src),
                     gst_structure_get_name (gst_message_get_structure(message)));
            break;
        }
        case GST_MESSAGE_NEW_CLOCK: {
            GstClock* clock = nullptr;
            gst_message_parse_new_clock(message, &clock);
            g_print ("New clock is %s.\n", clock->object.name);
            break;
        }
        case GST_MESSAGE_STREAM_START: {
            g_print("Stream is now started.\n");
            break;
        }
        case GST_MESSAGE_QOS:
        case GST_MESSAGE_STREAM_STATUS:
            break;
        default:
            g_printerr( "Unhandled message: %s\n", gst_message_type_get_name(message->type));
            break;
    }

    return TRUE;
}

#endif //GSTREAMER1_0_DRPAI_MESSAGES_H
