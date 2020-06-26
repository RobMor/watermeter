#ifndef PIPELINE_H
#define PIPELINE_H

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

class Pipeline {
private:
    GstElement *pipeline;
    GstAppSink *sink;
public:
    Pipeline();
    ~Pipeline();

    GdkPixbuf* Capture();
};

#endif
