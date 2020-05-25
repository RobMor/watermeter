#ifndef APP_H
#define APP_H

#include <math.h> // Simple math functions
#include <time.h> // Simple time functions
#include <assert.h> // Invariant assertions
#include <sys/stat.h> // Directory creation
#include <algorithm> // Max element algorithm

#include <gdk/gdk.h> // Tools for working with gtk
#include <gtk/gtk.h> // UI library
#include <gdk/gdkkeysyms.h> // Provides the key press macros
#include <gdk-pixbuf/gdk-pixbuf.h> // Functions for manipulating GDK Pixbufs
#include <cairo.h> // Drawing functions

#include "pipeline.h"

#define TED_PATH "~/ted/hourly/datacollection.py"

// number of angles around the circle to check average hue
#define NUM_ANGLES 512

// Number of seconds in between frames
#define FRAME_RATE 1

class Circle {
public:
    int x;
    int y;
    int r;

    void Draw(cairo_t *cr) {
        cairo_arc(cr, this->x, this->y, this->r, 0, 2 * M_PI);
    }
};

class Line {
public:
    int x1, y1, x2, y2;

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

    Circle *circle = new Circle();
    Line *line = new Line();
    Line *hoverLine = new Line();

    double histogram[NUM_ANGLES];

    bool isRunning = false;
    double currentReading = 0;
    double currentAngle = 0;
    int readingAtLastImageSave = 0;

    Pipeline *pipeline;
    
    GtkWidget *window;
    GtkWidget *drawingArea;
    GtkWidget *histogramArea;
    GtkWidget *numLabel;

    GdkPixbuf *image;

    GMainLoop *loop;

    void MakeWindow(int width, int height);
    
    void Refresh();

    static gboolean UpdateDrawingArea(GtkWidget *widget, cairo_t *cr, App *self);
    static gboolean UpdateHistogram(GtkWidget *widget, cairo_t *cr, App *self);
    static gboolean HistClick(GtkWidget *widget, GdkEventButton *event, App *self);
    static gboolean KeyPress(GtkWidget *widget, GdkEventKey *event, App *self);
    static gboolean ButtonPress(GtkWidget *widget, GdkEventButton *event, App *self);
    static gboolean ButtonRelease(GtkWidget *widget, GdkEventButton *event, App *self);
    static gboolean Motion(GtkWidget *widget, GdkEventMotion *event, App *self);
    
    guint frameTimeoutId = 0;
    static gboolean FrameTimeout(App *self);
    guint tedTimeoutId = 0;
    static gboolean TEDTimeout(App *self);

    void NextFrame();
    void FindNeedle();
    void ProcessFrame();
public:
    App(bool runTED, bool saveImages, bool saveAll);

    void Run();
    void Quit();
};

#endif