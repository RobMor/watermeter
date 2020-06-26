#ifndef PIPELINE_H
#define PIPELINE_H

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gst/app/gstappsink.h>
#include <gst/gst.h>

class Pipeline {
private:
    GstElement *pipeline;
    GstAppSink *sink;

public:
    Pipeline();
    ~Pipeline();

    GdkPixbuf *Capture();
};

#endif
