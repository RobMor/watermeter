#include <gst/app/gstappsink.h>

#include "main.h"
#include "ppm.h"

// if not defined, input will be from files listed in "ppmfiles.out"
#define INPUT_FROM_GST

int frame = -1;              // current frame

GstElement *pipeline;
GstAppSink *sink;

gboolean bushandler(GstBus*, GstMessage*, GtkWidget*);
void pixbufdestroy(guchar*, gpointer);

const char *imagefilenames[] = {
#ifndef INPUT_FROM_GST
#include "ppmfiles.out" // rather silly system, no?
#endif
};

/*
 *  === makepipeline ===
 *
 *  creates GST objects necessary for the pipeline, which gives us the video input
 */

void makepipeline(GtkWidget *window, image *mainimage) {
#ifdef INPUT_FROM_GST
    GstElement *source, *converter;
    GstCaps *caps;
    GstBus *bus;
    guint bus_watch_id;
    
    /* ------ Create pipeline ------ */
    pipeline = gst_pipeline_new("watermeter-webcam");
    
    /* ------ Create pipeline elements ------ */

    // Video4Linux source (what the webcam uses)
    source = gst_element_factory_make("v4l2src", "webcam");

    // A converter to convert the data from the webcam to any format we could
    // want (in this case we want raw rgb video)
    converter = gst_element_factory_make("videoconvert", "converter");

    // GstAppSink that we can get data from
    sink = GST_APP_SINK_CAST(gst_element_factory_make("appsink", "appsink"));
    // Only allow the AppSink to only store one buffer (aka frame)
    gst_app_sink_set_max_buffers(sink, 1);
    // Tell the AppSink to drop old buffers when we run out of space
    gst_app_sink_set_drop(sink, true);
    // We only want raw RGB data in this sink so we make caps to filter out
    // everything else
    caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", NULL);
    // Apply the caps to our sink
    gst_app_sink_set_caps(sink, caps);
    gst_caps_unref(caps); // GstCaps are refcounted...

    /* ------ Set up pipeline ------ */

    // Add a message handler to the pipeline (called bus handler in GST world)
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, (GstBusFunc) bushandler, NULL);
    g_object_unref(bus);

    // Add all previously created elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, converter, sink, NULL);

    // Link the elements together
    gst_element_link_many(source, converter, (GstElement*)sink, NULL);

    /* ------ Start the pipeline ------ */

    // Tell the pipeline to start playing
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait until the pipeline actually starts playing
    gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
#endif
}

/*
 *  === cleanuppipeline ===
 *
 *  free objects made in makepipeline
 */

void cleanuppipeline(byte *data, GdkPixbuf *pixbuf) {
#ifdef INPUT_FROM_GST
    // Stop the pipeline
    gst_element_set_state(pipeline, GST_STATE_NULL);
    // Free objects
    gst_object_unref(pipeline);
    gst_object_unref(sink);

    if (GDK_IS_PIXBUF(pixbuf)) g_object_unref(pixbuf);
#else
    free(data);
#endif
}

/*
 *  === capture ===
 *
 *  update mainpixbuf and mainimage.data to a more recent frame
 */

void capture(GdkPixbuf **pixbuf, image *mainimage) {
#ifdef INPUT_FROM_GST
    GstSample *sample;

    // Pull a sample (aka frame) from the sink blocking until one is available
    sample = gst_app_sink_pull_sample(sink);

    if (sample) {
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
            exit(-1);
        }
        
        // Get the structure behind the caps so we can grab height/width from
        // it
        s = gst_caps_get_structure(caps, 0);
        res |= gst_structure_get_int(s, "width", &width);
        res |= gst_structure_get_int(s, "height", &height);

        if (!res) {
            g_print("Could not get dimensions of frame\n");
            exit(-1);
        }
        
        // Get the buffer from the sample so we can access the raw data
        buffer = gst_sample_get_buffer(sample);

        // Map the buffer to map (dunno why we need to do this)
        if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            *pixbuf = gdk_pixbuf_new_from_data(map.data,
                    GDK_COLORSPACE_RGB, FALSE, 8, width, height,
                    GST_ROUND_UP_4 (width * 3), NULL, NULL);

            mainimage->data = map.data;

            gst_buffer_unmap(buffer, &map);
        }

        gst_sample_unref(sample);
    } else {
        if (gst_app_sink_is_eos(sink)) {
            g_print("Could not get frame because there are no more frames left\n");
        } else {
            g_print("Could not get frame (not EOS)\n");
        }
        exit(-1);
    }
#else
    
    // go to next frame
    frame++;
    if (frame == sizeof(imagefilenames)/sizeof(char*))
        frame--;
    
    // free the previous pixbuf if there is one
    if (GDK_IS_PIXBUF(*pixbuf)) g_object_unref(*pixbuf);
    
    // load the frame (see ppm.cpp)
    loadfile(imagefilenames[frame], mainimage);
    
    // create a new pixbuf
    *pixbuf = gdk_pixbuf_new_from_data(mainimage->data, GDK_COLORSPACE_RGB,
        FALSE, 8, mainimage->width, mainimage->height, mainimage->width*3,
        NULL, NULL);
    
#endif

}

/*
 *  === skipback ===
 *
 *  capture the previous frame instead of the next (should be called once before capture)
 */

void skipback() {
    frame -= 2;
    if (frame < -1)
        frame = -1;
}

// handles GST pipeline events
gboolean bushandler(GstBus *bus, GstMessage *msg, GtkWidget *window) {
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
            GError *err;
            gchar *debug;
            gst_message_parse_error(msg, &err, &debug);
            g_print("%s\n%s\n", err->message, debug);
            g_error_free(err);
            g_free(debug);
            quit();
            break;
    }
    return TRUE;
}

void pixbufdestroy(guchar *pixels, gpointer data) {
    gst_memory_unref((GstMemory*)data);
}
