#ifndef __APP_H_
#define __APP_H_

#include <cmath> // Simple math and PI
#include <cassert> // Invariant assertions
#include <sys/stat.h> // Directory creation TODO not portable

#include <gtkmm.h>

#include "config.h"
#include "pipeline.h"

class Circle {
public:
    double x, y, r;

    void Draw(const Cairo::RefPtr<Cairo::Context> &cr) {
        cr->save();
        cr->arc(x, y, r, 0, 2 * M_PI);
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
        cr->restore();
    }
};

class App : public Gtk::Application
{
public:
    static Glib::RefPtr<App> Create();

protected:
    App();

    int HandleCommandLine(const Glib::RefPtr<Gio::ApplicationCommandLine> &cmd);

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
    void NextFrame();
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

    sigc::connection frame_timeout_;
    sigc::connection ted_timeout_;

    Circle circle_;
    Line line_;

    bool save_images_;
    bool save_all_images_;
    bool run_ted_;

    bool save_debug_images_;
    bool save_hist_;

    bool running_;

    double reading_;
    double prev_reading_; // The reading at the last image save.
    double angle_;
    double angle_diff_; // The difference between the angle at the previous frame and this frame.

    double inner_; // Inner radius of needle detection
    double outer_; // Outer radius of needle detection

    double *hist_;
};

#endif
