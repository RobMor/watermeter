#ifndef __APP_H_
#define __APP_H_

#include <cmath> // Simple math and PI
#include <cassert> // Invariant assertions
#include <sys/stat.h> // Directory creation TODO not portable
#include <iostream> // printing

#include <gtkmm.h> // The GTK 3 C++ API. (Includes Cairo Gtk Gio and Glib)
#include <gstreamermm.h> // The GStreamer C++ API.

#include "config.h"
#include "webcam.h"

class Circle {
public:
    double x, y, r;

    void Draw(const Cairo::RefPtr<Cairo::Context> &cr) {
        cr->save();
        cr->arc(x, y, r, 0, 2 * M_PI);
        cr->stroke();
        cr->restore();
    }
};

class Line {
public:
    double x1, y1, x2, y2;

    void Draw(const Cairo::RefPtr<Cairo::Context> &cr) {
        cr->save();
        cr->move_to(x1, y1);
        cr->line_to(x2, y2);
        cr->stroke();
        cr->restore();
    }
};

class App : public Gtk::Application
{
public:
    static Glib::RefPtr<App> Create();

protected:
    App();

    int HandleCommandLine(const Glib::RefPtr<Glib::VariantDict> &cmd);

    void HandleActivate();
    bool HandleKeyPress(GdkEventKey *event);

    bool HandleDraw(const Cairo::RefPtr<Cairo::Context> &cr);
    bool HandleButtonPress(GdkEventButton *event);
    bool HandleMotion(GdkEventMotion *event);
    bool HandleButtonRelease(GdkEventButton *event);

    bool HandleFrameTimeout();
    bool HandleTEDTimeout();

private:
    void MakeWindow();
    Gst::FlowReturn HandleNewFrame();
    bool NewFrame();
    void Refresh();
    void FindNeedle();
    void AskForReading();
    double AngleDiff(double a, double b);
    void Output();

    void ComputeImagePosition(double &ratio, double &xoffset, double &yoffset);
    void DrawView(const Cairo::RefPtr<Cairo::Context> &cr);

    WebCam web_cam_;
    Glib::RefPtr<Gdk::Pixbuf> image_;

    Gtk::ApplicationWindow *window_;
    Gtk::DrawingArea *drawing_area_;
    Gtk::Label *label_;

    Glib::Mutex lock_;
    sigc::connection new_frame_;
    sigc::connection frame_timeout_;
    sigc::connection ted_timeout_;

    Circle circle_;
    Line line_;

    bool save_images_ = false;
    bool save_all_images_ = false;
    bool run_ted_ = false;

    bool save_debug_images_ = false;
    bool save_hist_ = false;

    bool running_ = false;

    double reading_ = 0;
    double prev_reading_ = 0; // The reading at the last image save.
    double angle_ = 0;
    double angle_diff_ = 0; // The difference between the angle at the previous frame and this frame.

    double inner_ = 0; // Inner radius of needle detection
    double outer_ = 1; // Outer radius of needle detection

    double *hist_;
};

#endif
