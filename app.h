#ifndef APP_H
#define APP_H

#include <assert.h>                // Invariant assertions
#include <cairo.h>                 // Drawing functions
#include <gdk-pixbuf/gdk-pixbuf.h> // Functions for manipulating GDK Pixbufs
#include <gdk/gdk.h>               // Tools for working with gtk
#include <gdk/gdkkeysyms.h>        // Provides the key press macros
#include <gtk/gtk.h>               // UI library
#include <math.h>                  // Simple math functions
#include <sys/stat.h>              // Directory creation
#include <time.h>                  // Simple time functions

#include <algorithm> // Max element algorithm

#include "config.h"
#include "pipeline.h"

class Circle {
public:
    double x;
    double y;
    double r;

    void Draw(cairo_t *cr) {
        cairo_arc(cr, this->x, this->y, this->r, 0, 2 * M_PI);
    }
};

class Line {
public:
    double x1, y1, x2, y2;

    void Draw(cairo_t *cr) {
        cairo_move_to(cr, this->x1, this->y1);
        cairo_line_to(cr, this->x2, this->y2);
    }
};

class App {
private:
    bool runTED;
    bool saveImages;
    bool saveAll;
    bool saveDebug;
    bool saveHist;

    Circle *circle = new Circle();
    Line *line = new Line();

    bool isRunning = false;
    float start = 0;
    float end = 1;
    double currentReading = 0;
    double currentAngle = 0;
    double readingAtLastImageSave = 0;

    double hist[NUM_ANGLES];

    Pipeline *pipeline;

    GtkApplication *app;
    GtkWidget *window;
    GtkWidget *drawingArea;
    GtkWidget *numLabel;

    GdkPixbuf *image;

    static void Activate(GApplication *app, App *self);

    static gboolean UpdateDrawingArea(GtkWidget *widget, cairo_t *cr,
                                      App *self);
    static gboolean KeyPress(GtkWidget *widget, GdkEventKey *event, App *self);
    static gboolean ButtonPress(GtkWidget *widget, GdkEventButton *event,
                                App *self);
    static gboolean ButtonRelease(GtkWidget *widget, GdkEventButton *event,
                                  App *self);
    static gboolean Motion(GtkWidget *widget, GdkEventMotion *event, App *self);

    void MakeWindow();
    void ComputeImagePosition(double *ratio, double *xoffset, double *yoffset);
    void AskForReading();
    void Refresh();
    void NextFrame();
    void FindNeedle();
    void DrawImage(cairo_t *cr);
    void ProcessFrame();

    guint frameTimeoutId = 0;
    static gboolean FrameTimeout(App *self);
    guint tedTimeoutId = 0;
    static gboolean TEDTimeout(App *self);

public:
    App(bool runTED, bool saveImages, bool saveAll, bool saveDebug, bool saveHist);

    void Run();
    void Quit();
};

#endif
