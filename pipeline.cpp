#include "pipeline.h"

gboolean BusHandler(GstBus *bus, GstMessage *msg, gpointer *data);

Pipeline::Pipeline() {
    GstElement *source, *converter;
    GstCaps *caps;
    GstBus *bus;

    /* ------ Create pipeline ------ */
    this->pipeline = gst_pipeline_new("watermeter-webcam");

    /* ------ Create pipeline elements ------ */

    // Video4Linux source (what the webcam uses)
    source = gst_element_factory_make("v4l2src", "webcam");

    // A converter to convert the data from the webcam to any format we could
    // want (in this case we want raw rgb video)
    converter = gst_element_factory_make("videoconvert", "converter");

    // GstAppSink that we can get data from
    this->sink =
        GST_APP_SINK_CAST(gst_element_factory_make("appsink", "appsink"));
    // Only allow the AppSink to only store one buffer (aka frame)
    gst_app_sink_set_max_buffers(sink, 1);
    // Tell the AppSink to drop old buffers when we run out of space
    gst_app_sink_set_drop(sink, true);
    // We only want raw RGB data in this sink so we make caps to filter out
    // everything else
    caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB",
                               NULL);
    // Apply the caps to our sink
    gst_app_sink_set_caps(sink, caps);
    gst_caps_unref(caps); // GstCaps are refcounted...

    /* ------ Set up pipeline ------ */

    // Add a message handler to the pipeline (called bus handler in GST world)
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, (GstBusFunc)BusHandler, NULL);
    g_object_unref(bus);

    // Add all previously created elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, converter, sink, NULL);

    // Link the elements together
    gst_element_link_many(source, converter, (GstElement *)sink, NULL);

    /* ------ Start the pipeline ------ */

    // Tell the pipeline to start playing
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait until the pipeline actually starts playing
    gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
}

Pipeline::~Pipeline() {
    // Stop the pipeline
    gst_element_set_state(this->pipeline, GST_STATE_NULL);
    // Free objects
    gst_object_unref(this->pipeline);
    gst_object_unref(this->sink);
}

GdkPixbuf *Pipeline::Capture() {
    GstSample *sample;

    // Pull a sample (aka frame) from the sink blocking until one is available
    sample = gst_app_sink_pull_sample(sink);

    if (sample) {
        GdkPixbuf *pixbuf;
        GstBuffer *buffer;
        GstCaps *caps;
        GstStructure *s;
        gint width, height;
        GstMapInfo map;
        gboolean res = true;

        // Get the caps from the sample
        caps = gst_sample_get_caps(sample);

        if (!caps) {
            g_print("Could not get format of frame\n");
            exit(1);
        }

        // Get the structure behind the caps so we can grab height/width from
        // it
        s = gst_caps_get_structure(caps, 0);
        res |= gst_structure_get_int(s, "width", &width);
        res |= gst_structure_get_int(s, "height", &height);

        if (!res) {
            g_print("Could not get dimensions of frame\n");
            exit(1);
        }

        // Get the buffer from the sample so we can access the raw data
        buffer = gst_sample_get_buffer(sample);

        // Map the buffer to map (dunno why we need to do this)
        if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            pixbuf = gdk_pixbuf_new_from_data(
                map.data, GDK_COLORSPACE_RGB, FALSE, 8, width, height,
                GST_ROUND_UP_4(width * 3), NULL, NULL);

            gst_buffer_unmap(buffer, &map);
        } else {
            pixbuf = NULL;
        }

        gst_sample_unref(sample);

        return pixbuf;
    } else {
        if (gst_app_sink_is_eos(sink)) {
            g_print(
                "Could not get frame because there are no more frames left\n");
        } else {
            g_print("Could not get frame (not EOS)\n");
        }
        exit(1);
    }
}

// A callback for the GStreamer bus. Basically handles any errors that occur.
gboolean BusHandler(GstBus *bus, GstMessage *msg, gpointer *data) {
    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR:
        GError *err;
        gchar *debug;
        gst_message_parse_error(msg, &err, &debug);
        g_print("%s\n%s\n", err->message, debug);
        g_error_free(err);
        g_free(debug);
        exit(1);
        break;
    default:
        return TRUE;
    }
}
