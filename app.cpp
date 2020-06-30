#include "app.h"

App::App(bool runTED, bool saveImages, bool saveAll, bool saveDebug, bool saveHist) {
    this->runTED = runTED;
    this->saveImages = saveImages;
    this->saveAll = saveAll;
    this->saveDebug = saveDebug;
    this->saveHist = saveHist;

    this->pipeline = new Pipeline();

    this->image = NULL;

    this->NextFrame();

    this->app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);

    g_signal_connect(G_APPLICATION(this->app), "activate",
                     G_CALLBACK(App::Activate), this);
}

void App::Run() {
    g_application_run(G_APPLICATION(this->app), 0, NULL);

    g_object_unref(this->app);

    // TODO more cleanup
}

void App::Activate(GApplication *app, App *self) {
    self->MakeWindow();
    self->Refresh();
}

void App::MakeWindow() {
    this->window = gtk_application_window_new(GTK_APPLICATION(this->app));
    gtk_window_set_default_size(GTK_WINDOW(this->window), 640, 360);

    g_signal_connect(this->window, "key-press-event", G_CALLBACK(App::KeyPress),
                     this);

    this->drawingArea = gtk_drawing_area_new();
    g_signal_connect(this->drawingArea, "draw",
                     G_CALLBACK(App::UpdateDrawingArea), this);
    g_signal_connect(this->drawingArea, "button-press-event",
                     G_CALLBACK(App::ButtonPress), this);
    g_signal_connect(this->drawingArea, "button-release-event",
                     G_CALLBACK(App::ButtonRelease), this);
    g_signal_connect(this->drawingArea, "motion-notify-event",
                     G_CALLBACK(App::Motion), this);

    // Set a minimum size
    gtk_widget_set_size_request(this->drawingArea, 100, 100);

    gtk_widget_add_events(this->drawingArea, GDK_BUTTON_PRESS_MASK |
                                                 GDK_BUTTON_RELEASE_MASK |
                                                 GDK_BUTTON_MOTION_MASK);

    this->numLabel = gtk_label_new(NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

    gtk_box_pack_start(GTK_BOX(vbox), this->drawingArea, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), this->numLabel, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(this->window), vbox);

    gtk_widget_show_all(this->window);
}

void App::Refresh() {
    char *label;

    if (this->isRunning) {
        asprintf(&label, "RUNNING - Current Reading: %010.2f",
                 this->currentReading);
    } else {
        asprintf(&label, "PAUSED - Current Reading: %010.2f",
                 this->currentReading);
    }

    gtk_label_set_text(GTK_LABEL(this->numLabel), label);

    free(label);

    gdk_window_invalidate_rect(gtk_widget_get_window(this->window), NULL,
                               FALSE);
}

void App::ComputeImagePosition(double *ratio, double *xoffset,
                               double *yoffset) {
    guint imageWidth = gdk_pixbuf_get_width(this->image);
    guint imageHeight = gdk_pixbuf_get_height(this->image);

    guint areaWidth = gtk_widget_get_allocated_width(this->drawingArea);
    guint areaHeight = gtk_widget_get_allocated_height(this->drawingArea);

    double widthRatio = (double)areaWidth / (double)imageWidth;
    double heightRatio = (double)areaHeight / (double)imageHeight;

    *ratio = fmin(widthRatio, heightRatio);

    double extraWidth = (double)areaWidth - (imageWidth * *ratio);
    double extraHeight = (double)areaHeight - (imageHeight * *ratio);

    *xoffset = (extraWidth / 2);
    *yoffset = (extraHeight / 2);
}

gboolean App::UpdateDrawingArea(GtkWidget *widget, cairo_t *cr, App *self) {
    double ratio, xoffset, yoffset;
    self->ComputeImagePosition(&ratio, &xoffset, &yoffset);

    cairo_save(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_rectangle(cr, 0, 0, gtk_widget_get_allocated_width(widget),
                    gtk_widget_get_allocated_height(widget));
    cairo_fill(cr);
    cairo_stroke(cr);

    cairo_translate(cr, xoffset, yoffset);
    cairo_scale(cr, ratio, ratio);

    self->DrawImage(cr);

    cairo_restore(cr);

    return TRUE;
}

void App::DrawImage(cairo_t *cr) {
    cairo_save(cr);

    cairo_surface_t *surface = gdk_cairo_surface_create_from_pixbuf(
        this->image, 1, gtk_widget_get_window(this->window));
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);

    cairo_set_source_rgb(cr, 1, 0, 0);
    this->circle->Draw(cr);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0, 1, 0);
    this->line->Draw(cr);
    cairo_stroke(cr);

    cairo_restore(cr);
}

gboolean App::KeyPress(GtkWidget *widget, GdkEventKey *event, App *self) {
    if (event->type != GDK_KEY_PRESS)
        return FALSE;

    if (self->isRunning) {
        switch (event->keyval) {
        // Pause
        case GDK_KEY_space:
            self->isRunning = false;

            if (self->frameTimeoutId != 0) {
                g_source_remove(self->frameTimeoutId);
                self->frameTimeoutId = 0;
            }
            if (self->runTED && self->frameTimeoutId != 0) {
                g_source_remove(self->tedTimeoutId);
                self->tedTimeoutId = 0;
            }

            self->Refresh();
            return TRUE;
        default:
            return FALSE;
        }
    } else {
        switch (event->keyval) {
        // Play
        case GDK_KEY_space: {
            self->isRunning = true;

            self->NextFrame();
            self->ProcessFrame();

            if (self->frameTimeoutId != 0) {
                g_source_remove(self->frameTimeoutId);
            }
            self->frameTimeoutId =
                g_timeout_add(FRAME_RATE, (GSourceFunc)App::FrameTimeout, self);
            if (self->runTED) {
                if (self->tedTimeoutId != 0) {
                    g_source_remove(self->tedTimeoutId);
                }
                self->tedTimeoutId = g_timeout_add_seconds(
                    3600, (GSourceFunc)App::TEDTimeout, self);
            }

            self->Refresh();
            return TRUE;
        }
        // Update the current frame
        case GDK_KEY_Return: {
            self->NextFrame();
            self->ProcessFrame();
            self->Refresh();

            return TRUE;
        }
        // Move needle detection up
        case GDK_KEY_j: {
            if ((event->state & gtk_accelerator_get_default_mod_mask()) ==
                GDK_CONTROL_MASK) {
                self->start = fmin(self->end, self->start + 0.1);
            } else {
                self->end = fmin(1, self->end + 0.1);
            }

            self->FindNeedle();
            self->Refresh();
            return TRUE;
        }
        // Move needle detection down
        case GDK_KEY_k: {
            if ((event->state & gtk_accelerator_get_default_mod_mask()) ==
                GDK_CONTROL_MASK) {
                self->start = fmax(0, self->start - 0.1);
            } else {
                self->end = fmax(self->start, self->end - 0.1);
            }

            self->FindNeedle();
            self->Refresh();
            return TRUE;
        }
        // Ask the user for the current reading
        case GDK_KEY_r: {
            self->AskForReading();
            self->Refresh();
            return TRUE;
        }
        default: {
            return FALSE;
        }
        }
    }
}

gboolean App::ButtonPress(GtkWidget *widget, GdkEventButton *event, App *self) {
    switch (event->button) {
    // left button pressed
    case 1:
        if (!self->isRunning) {
            double ratio, xoffset, yoffset;
            self->ComputeImagePosition(&ratio, &xoffset, &yoffset);

            double x = ((double)event->x - xoffset) / ratio;
            double y = ((double)event->y - yoffset) / ratio;

            // start dragging the circle
            self->circle->x = x;
            self->circle->y = y;
            self->circle->r = 0;

            self->line = new Line();
        }
        return TRUE;
    default:
        return FALSE;
    }
}

gboolean App::ButtonRelease(GtkWidget *widget, GdkEventButton *event,
                            App *self) {
    switch (event->button) {
    // left button released
    case 1:
        if (!self->isRunning) {
            self->FindNeedle();
            self->Refresh();
        }
        return TRUE;
    default:
        return FALSE;
    }
}

gboolean App::Motion(GtkWidget *widget, GdkEventMotion *event, App *self) {
    if (!self->isRunning) {
        // left button is pressed; set new circle radius
        if (event->state & GDK_BUTTON1_MASK) {
            double ratio, xoffset, yoffset;
            self->ComputeImagePosition(&ratio, &xoffset, &yoffset);

            double x = ((double)event->x - xoffset) / ratio;
            double y = ((double)event->y - yoffset) / ratio;

            int dx = x - self->circle->x;
            int dy = y - self->circle->y;

            self->circle->r = sqrt(dx * dx + dy * dy);

            self->Refresh();
            return TRUE;
        }
    }
    return FALSE;
}

void App::AskForReading() {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Reading", GTK_WINDOW(this->window), GTK_DIALOG_MODAL, "_OK",
        GTK_RESPONSE_OK, NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *label = gtk_label_new("Current reading of dial:");

    GtkWidget *entry = gtk_entry_new();

    char *currentReading;
    asprintf(&currentReading, "%f", this->currentReading);
    gtk_entry_set_text(GTK_ENTRY(entry), currentReading);

    gtk_container_add(GTK_CONTAINER(content_area), label);
    gtk_container_add(GTK_CONTAINER(content_area), entry);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));

    const char *input = gtk_entry_get_text(GTK_ENTRY(entry));
    this->currentReading = atof(input);

    gtk_widget_destroy(dialog);
}

// Look around the user placed circle trying to find at which angle the
// needle is currently pointing.
void App::FindNeedle() {
    if (this->circle->r <= 0)
        return;

    assert(gdk_pixbuf_get_colorspace(this->image) == GDK_COLORSPACE_RGB);
    assert(gdk_pixbuf_get_n_channels(this->image) == 3);

    const int width = gdk_pixbuf_get_width(this->image);
    const int height = gdk_pixbuf_get_height(this->image);
    const int row_stride = gdk_pixbuf_get_rowstride(this->image);
    const guint8 *image = gdk_pixbuf_read_pixels(this->image);

    double max_redness = NAN;
    double max_redness_angle = NAN;

    // Iterate angles
    for (int t = 0; t < NUM_ANGLES; t++) {
        double angle = (2 * M_PI) * ((double)t / NUM_ANGLES);
        double cosa = std::cos(angle);
        double sina = std::sin(angle);

        double sum = 0;
        // Iterate radii
        for (int d = this->start * this->circle->r;
             d < this->end * this->circle->r; d++) {
            int x = this->circle->x + (d * cosa);
            int y = this->circle->y + (d * sina);

            if (x >= 0 && x < width && y >= 0 && y < height) {
                int index = (row_stride * y) + (x * 3);

                int r = image[index];
                int g = image[index + 1];
                int b = image[index + 2];

                // TODO could cause division by zero errors when encountering
                // pure red...

                // Redness metric. This came from the previous version of the
                // program. It seems to work pretty well.
                sum += (double)(2 * r) / (double)(b + g);
            } else {
                break;
            }
        }

        double redness = sum / (double)this->circle->r;

        if (this->saveHist)
            this->hist[t] = redness;

        if (isnan(max_redness) || redness > max_redness) {
            max_redness = redness;
            max_redness_angle = angle;
        }
    }

    assert(!isnan(max_redness));

    this->currentAngle = max_redness_angle;

    double cos_needle = std::cos(max_redness_angle);
    double sin_needle = std::sin(max_redness_angle);

    this->line->x1 =
        this->circle->x + this->start * (this->circle->r * cos_needle);
    this->line->y1 =
        this->circle->y + this->start * (this->circle->r * sin_needle);

    this->line->x2 =
        this->circle->x + this->end * (this->circle->r * cos_needle);
    this->line->y2 =
        this->circle->y + this->end * (this->circle->r * sin_needle);
}

void App::NextFrame() {
    if (this->image)
        g_object_unref(this->image);
    this->image = this->pipeline->Capture();
}

double angle_diff(double a, double b) {
    double diff1 = (b - a);
    double diff2 = diff1 + 2 * M_PI;
    double diff3 = diff1 - 2 * M_PI;

    double absdiff1 = fabs(diff1);
    double absdiff2 = fabs(diff2);
    double absdiff3 = fabs(diff3);

    if (absdiff1 <= absdiff2 && absdiff1 <= absdiff3)
        return diff1;
    else if (absdiff2 <= absdiff1 && absdiff2 <= absdiff3)
        return diff2;
    else // if (absdiff3 <= absdiff1 && absdiff3 <= absdiff2)
        return diff3;
}

void App::ProcessFrame() {
    // TODO make sure this is accurate.

    double prevAngle = this->currentAngle;
    this->FindNeedle();
    double angleChange = angle_diff(prevAngle, this->currentAngle);

    // One full rotation of the dial is 10 gallons
    this->currentReading += angleChange * (10 / (2 * M_PI));

    time_t now;
    time(&now);

    char dateString[64];
    strftime(dateString, 64, "%Y-%m-%d %H:%M:%S", localtime(&now));

    FILE *outfile = fopen(DATA_FILE, "a");
    fprintf(outfile, "%s,%f\n", dateString, this->currentReading);
    fclose(outfile);

    if (this->saveHist) {
        FILE *histfile = fopen(HIST_FILE, "a");
        fprintf(histfile, "%s", dateString);
        for (int i = 0; i < NUM_ANGLES; i++)
            fprintf(histfile, ",%f", this->hist[i]);
        fprintf(histfile, "\n");
        fclose(histfile);
    }

    if (this->saveAll ||
            (this->saveImages && fabs(this->readingAtLastImageSave - this->currentReading) > 10)) {
        this->readingAtLastImageSave = this->currentReading;

        char fileName[64];
        mkdir(IMAGES_FOLDER, 0777);
        strftime(fileName, 64, IMAGES_FOLDER "/%Y-%m-%dT%H_%M_%S.jpg",
                 localtime(&now));
        
        gdk_pixbuf_save(this->image, fileName, "jpeg", NULL, NULL);
    }

    if (this->saveDebug) {
        char fileName[64];
        mkdir(DEBUG_FOLDER, 0777);
        strftime(fileName, 64, DEBUG_FOLDER "/%Y-%m-%dT%H_%M_%S.jpg", localtime(&now));

        int width = gdk_pixbuf_get_width(this->image);
        int height = gdk_pixbuf_get_height(this->image);

        cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
        cairo_t *cr = cairo_create(surface);

        this->DrawImage(cr);

        cairo_surface_flush(surface);

        GdkPixbuf *drawnImage = gdk_pixbuf_get_from_surface(surface, 0, 0, width, height);

        gdk_pixbuf_save(drawnImage, fileName, "jpeg", NULL, NULL);

        g_object_unref(drawnImage);
        cairo_surface_destroy(surface);
        cairo_destroy(cr);
    }
}

gboolean App::FrameTimeout(App *self) {
    if (self->isRunning) {
        self->NextFrame();
        self->ProcessFrame();
        self->Refresh();
    }

    return self->isRunning;
}

gboolean App::TEDTimeout(App *self) {
    if (self->isRunning && self->runTED) {
        system(TED_PATH " auto");
    }

    return self->isRunning;
}
