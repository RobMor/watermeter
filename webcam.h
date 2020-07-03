#ifndef __PIPELINE_H
#define __PIPELINE_H

#include <gdkmm.h>
#include <gstreamermm.h>

class WebCam {
public:
    WebCam();
    ~WebCam();

    void Init();
    Glib::RefPtr<Gdk::Pixbuf> Capture();

protected:
    bool BusHandler(const Glib::RefPtr<Gst::Bus> &bus, const Glib::RefPtr<Gst::Message> &message);

private:
    Glib::RefPtr<Gst::Pipeline> pipeline_;
    Glib::RefPtr<Gst::AppSink> sink_;
};

#endif
