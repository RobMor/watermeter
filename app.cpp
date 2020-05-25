#include "app.h"

App::App(bool runTED, bool saveImages, bool saveAll)
{
    this->runTED = runTED;
    this->saveImages = saveImages;
    this->saveAll = saveAll;

    this->pipeline = new Pipeline();

    this->NextFrame();

    this->MakeWindow(gdk_pixbuf_get_width(this->image), gdk_pixbuf_get_height(this->image));

    loop = g_main_loop_new(NULL, FALSE);
}

void App::Run()
{
    this->Refresh();

    g_main_loop_run(this->loop);
}

void App::MakeWindow(int width, int height)
{
    this->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    // TEMPORARY FOR TESTING
    gtk_window_set_type_hint(GTK_WINDOW(this->window), GDK_WINDOW_TYPE_HINT_DIALOG);

    g_signal_connect(this->window, "key-press-event", G_CALLBACK(App::KeyPress), this);

    this->drawingArea = gtk_drawing_area_new();
    g_signal_connect(this->drawingArea, "draw", G_CALLBACK(App::UpdateDrawingArea), this);
    g_signal_connect(this->drawingArea, "button-press-event", G_CALLBACK(App::ButtonPress), this);
    g_signal_connect(this->drawingArea, "button-release-event", G_CALLBACK(App::ButtonRelease), this);
    g_signal_connect(this->drawingArea, "motion-notify-event", G_CALLBACK(App::Motion), this);

    gtk_widget_set_size_request(this->drawingArea, width, height);

    gtk_widget_add_events(this->drawingArea,
                          GDK_BUTTON_PRESS_MASK |
                              GDK_BUTTON_RELEASE_MASK |
                              GDK_BUTTON_MOTION_MASK);

    this->numLabel = gtk_label_new("");

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

    gtk_box_pack_start(GTK_BOX(vbox), this->drawingArea, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), numLabel, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(this->window), vbox);

    gtk_widget_show_all(this->window);
}

void App::Refresh()
{
    char *label;

    if (this->isRunning)
    {
        asprintf(&label, "RUNNING - Current Reading: %09.1f", this->currentReading);
    }
    else
    {
        asprintf(&label, "PAUSED - Current Reading: %09.1f", this->currentReading);
    }

    gtk_label_set_text(GTK_LABEL(numLabel), label);

    free(label);

    gdk_window_invalidate_rect(gtk_widget_get_window(this->window), NULL, FALSE);
}

gboolean App::UpdateDrawingArea(GtkWidget *widget, cairo_t *cr, App *self)
{
    cairo_save(cr);

    cairo_set_source_surface(cr, gdk_cairo_surface_create_from_pixbuf(self->image, 1, gtk_widget_get_window(self->window)), 0, 0);
    cairo_rectangle(cr, 0, 0, gdk_pixbuf_get_width(self->image), gdk_pixbuf_get_height(self->image));
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 1, 0, 0);
    self->circle->Draw(cr);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0, 1, 0);
    self->line->Draw(cr);
    cairo_stroke(cr);

    cairo_restore(cr);

    return TRUE;
}

gboolean App::KeyPress(GtkWidget *widget, GdkEventKey *event, App *self)
{
    if (event->type != GDK_KEY_PRESS)
        return FALSE;

    if (self->isRunning)
    {
        switch (event->keyval)
        {
        // Pause
        case GDK_KEY_space:
            self->isRunning = false;

            if (self->frameTimeoutId != 0)
            {
                g_source_remove(self->frameTimeoutId);
                self->frameTimeoutId = 0;
            }
            if (self->runTED && self->frameTimeoutId != 0)
            {
                g_source_remove(self->tedTimeoutId);
                self->tedTimeoutId = 0;
            }

            self->Refresh();
            return TRUE;
        default:
            return FALSE;
        }
    }
    else
    {
        switch (event->keyval)
        {
        // Play
        case GDK_KEY_space:
        {
            self->isRunning = true;

            self->NextFrame();
            self->ProcessFrame();

            if (self->frameTimeoutId != 0)
            {
                g_source_remove(self->frameTimeoutId);
            }
            self->frameTimeoutId = g_timeout_add(FRAME_RATE, G_SOURCE_FUNC(App::FrameTimeout), self);
            if (self->runTED)
            {
                if (self->tedTimeoutId != 0)
                {
                    g_source_remove(self->tedTimeoutId);
                }
                self->tedTimeoutId = g_timeout_add_seconds(3600, G_SOURCE_FUNC(App::TEDTimeout), self);
            }

            self->Refresh();
            return TRUE;
        }
        // Ask the user for the current reading
        case GDK_KEY_r:
        {
            self->AskForReading();
            self->Refresh();
            return TRUE;
        }
        default:
        {
            return FALSE;
        }
        }
    }
}

gboolean App::ButtonPress(GtkWidget *widget, GdkEventButton *event, App *self)
{
    switch (event->button)
    {
    // left button pressed
    case 1:
        if (!self->isRunning)
        {
            // start dragging the circle
            self->circle->x = event->x;
            self->circle->y = event->y;
            self->circle->r = 0;

            self->line = new Line();
        }
        return TRUE;
    default:
        return FALSE;
    }
}

gboolean App::ButtonRelease(GtkWidget *widget, GdkEventButton *event, App *self)
{
    switch (event->button)
    {
    // left button released
    case 1:
        if (!self->isRunning)
        {
            self->FindNeedle();
            self->Refresh();
        }
        return TRUE;
    default:
        return FALSE;
    }
}

gboolean App::Motion(GtkWidget *widget, GdkEventMotion *event, App *self)
{
    if (!self->isRunning)
    {
        // left button is pressed; set new circle radius
        if (event->state & GDK_BUTTON1_MASK)
        {
            int dx = event->x - self->circle->x;
            int dy = event->y - self->circle->y;

            self->circle->r = sqrt(dx * dx + dy * dy);

            self->Refresh();
            return TRUE;
        }
    }
    return FALSE;
}

void App::AskForReading()
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Reading", GTK_WINDOW(this->window), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *label = gtk_label_new("Current whole number reading of dial:");

    GtkWidget *spin = gtk_spin_button_new_with_range(0, 9999999, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), this->currentReading);

    gtk_container_add(GTK_CONTAINER(content_area), label);
    gtk_container_add(GTK_CONTAINER(content_area), spin);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_spin_button_update(GTK_SPIN_BUTTON(spin));
    this->currentReading = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));

    gtk_widget_destroy(dialog);
}

// Look around the user placed circle trying to find at which angle the
// needle is currently pointing.
void App::FindNeedle()
{
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
    for (int t = 0; t < NUM_ANGLES; t++)
    {
        double angle = (2 * M_PI) * ((double)t / NUM_ANGLES);
        double cosa = std::cos(angle);
        double sina = std::sin(angle);

        double sum = 0;
        // Iterate radii
        for (int d = 0; d < this->circle->r; d++)
        {
            int x = this->circle->x + (d * cosa);
            int y = this->circle->y + (d * sina);

            if (x >= 0 && x < width && y >= 0 && y < height)
            {
                int index = (row_stride * y) + (x * 3);

                int r = image[index];
                int g = image[index + 1];
                int b = image[index + 2];

                // Redness metric. This came from the previous version of the
                // program. It seems to work pretty well.
                sum += (double)(2 * r + b + g) / 4;
            }
            else
            {
                break;
            }
        }

        double redness = sum / (double)this->circle->r;

        if (isnan(max_redness) || redness < max_redness)
        {
            max_redness = redness;
            max_redness_angle = angle;
        }
    }

    assert(!isnan(max_redness));

    this->currentAngle = max_redness_angle;

    double cos_needle = std::cos(max_redness_angle);
    double sin_needle = std::sin(max_redness_angle);

    this->line->x1 = this->circle->x;
    this->line->y1 = this->circle->y;

    this->line->x2 = this->circle->x + (this->circle->r * cos_needle);
    this->line->y2 = this->circle->y + (this->circle->r * sin_needle);
}

void App::NextFrame()
{
    this->image = this->pipeline->Capture();
}

void App::ProcessFrame()
{
    // TODO make sure this is accurate.

    double prevAngle = this->currentAngle;
    this->FindNeedle();
    int angleChange = this->currentAngle - prevAngle;
    // Account for angle wrapping when computing the change
    angleChange = fmod(angleChange, 2 * M_PI);
    angleChange = fmin(2 * M_PI - angleChange, angleChange);

    // One full rotation of the dial is 10 gallons
    this->currentReading += angleChange * (10 / (2 * M_PI));

    time_t now;
    time(&now);

    char dateString[64];
    strftime(dateString, 64, "%Y-%m-%d %H:%M:%S", localtime(&now));

    FILE *outfile = fopen(DATA_FILE, "a");
    fprintf(outfile, "%s,%f\n", dateString, this->currentReading);
    fclose(outfile);

    if (this->saveImages && (this->saveAll || fabs(this->readingAtLastImageSave - this->currentReading) > 10))
    {
        this->readingAtLastImageSave = this->currentReading;

        char fileName[64];
        mkdir(IMAGES_FOLDER, 0777);
        strftime(fileName, 64, IMAGES_FOLDER "/%Y-%m-%dT%H_%M_%S.jpg", localtime(&now));

        gdk_pixbuf_save(this->image, fileName, "jpeg", NULL, NULL);
    }
}

gboolean App::FrameTimeout(App *self)
{
    if (self->isRunning)
    {
        self->NextFrame();
        self->ProcessFrame();
        self->Refresh();
    }

    return self->isRunning;
}

gboolean App::TEDTimeout(App *self)
{
    if (self->isRunning && self->runTED)
    {
        int exitCode = system(TED_PATH " auto");
    }

    return self->isRunning;
}