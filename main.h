#ifndef main_h
#define main_h

#ifdef main_cpp
#define EXTERN
#else
#define EXTERN extern
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <gst/gst.h>

typedef unsigned char byte;

typedef struct {
    byte* data;
    int width, height;
} image;

EXTERN int histdata[256];
EXTERN int thresholdoffset;

void quit();

#endif
