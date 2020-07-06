#include <iomanip>

#include "app.h"

App::App() : Gtk::Application("wm.app") {
}

Glib::RefPtr<App> App::Create() {
    App *app = new App();

    app->add_main_option_entry(Gio::Application::OptionType::OPTION_TYPE_BOOL, "save", 's', "Save the current frame every full rotation of the dial");
    app->add_main_option_entry(Gio::Application::OptionType::OPTION_TYPE_BOOL, "save-all", 'a', "Save every frame");
    app->add_main_option_entry(Gio::Application::OptionType::OPTION_TYPE_BOOL, "run-ted", 't', "Run the TED data collection program every hour");
    app->add_main_option_entry(Gio::Application::OptionType::OPTION_TYPE_BOOL, "save-debug", 'd', "Save debug images every frame");
    app->add_main_option_entry(Gio::Application::OptionType::OPTION_TYPE_BOOL, "save-hist", 'h', "Save a histogram of needle detection data every frame");

    app->signal_handle_local_options().connect(sigc::mem_fun(app, &App::HandleCommandLine));
    app->signal_activate().connect(sigc::mem_fun(app, &App::HandleActivate));

    return Glib::RefPtr<App>(app);
}

int App::HandleCommandLine(const Glib::RefPtr<Glib::VariantDict> &options) {
    save_images_ = options->contains("save");
    save_all_images_ = options->contains("save-all");
    run_ted_ = options->contains("run-ted");
    save_debug_images_ = options->contains("save-debug");
    save_hist_ = options->contains("save-hist");

    return -1;
}

void App::HandleActivate() {
    web_cam_.signal_new_frame().connect(sigc::mem_fun(this, &App::HandleNewFrame));
    web_cam_.Init();

    if (save_hist_)
        hist_ = new double[NUM_ANGLES];
    
    MakeWindow();
    NewFrame();
    Refresh();

    window_->show_all();
    window_->present();
}

void App::MakeWindow() {
    window_ = new Gtk::ApplicationWindow();
    add_window(*window_);
    window_->set_default_size(640, 360);
    // after=false because some other signal handler is intercepting the signal
    // on the space key (???)
    window_->signal_key_press_event().connect(sigc::mem_fun(this, &App::HandleKeyPress), false);

    Gtk::VBox *vbox = new Gtk::VBox(false, 2);
    window_->add(*vbox);

    drawing_area_ = new Gtk::DrawingArea();
    vbox->pack_start(*drawing_area_, Gtk::PackOptions::PACK_EXPAND_WIDGET);
    drawing_area_->set_size_request(100, 100);
    drawing_area_->add_events(Gdk::EventMask::BUTTON_PRESS_MASK | Gdk::EventMask::BUTTON_MOTION_MASK);
    drawing_area_->signal_draw().connect(sigc::mem_fun(this, &App::HandleDraw));
    drawing_area_->signal_button_press_event().connect(sigc::mem_fun(this, &App::HandleButtonPress));
    drawing_area_->signal_motion_notify_event().connect(sigc::mem_fun(this, &App::HandleMotion));

    label_ = new Gtk::Label();
    vbox->pack_start(*label_, Gtk::PackOptions::PACK_SHRINK);
}

Gst::FlowReturn App::HandleNewFrame() {
    // This callback only gets called from the gstreamer thread (of which there
    // is only one [?]) so depending on a raw if statement here to register the
    // callback should be fine (no need for a mutex)
    if (!new_frame_) {
        new_frame_ = Glib::signal_idle().connect(sigc::mem_fun(this, &App::NewFrame));
    }

    return Gst::FlowReturn::FLOW_OK;
}

bool App::NewFrame() {
    // TODO does the refptr clean itself up?
    image_ = web_cam_.Capture();
    
    FindNeedle();
    Refresh();

    // Don't want this signal handler to be called over and over.
    return false;
}

void App::Refresh() {
    char *label_text;

    if (running_) {
        asprintf(&label_text, "RUNNING - Current Reading: %010.2f", reading_);
    } else {
        asprintf(&label_text, "STOPPED - Current Reading: %010.2f", reading_);
    }

    label_->set_text(label_text);

    free(label_text);

    // TODO necessary?
    window_->queue_draw();
}

bool App::HandleDraw(const Cairo::RefPtr<Cairo::Context>& cr) {
    cr->save();

    // Fill background with black
    cr->set_source_rgb(0, 0, 0);
    cr->rectangle(0, 0, drawing_area_->get_allocated_width(), drawing_area_->get_allocated_height());
    cr->fill();
    cr->stroke();

    cr->restore();

    cr->save();
    
    // Scale and center the main camera view
    double ratio, xoffset, yoffset;
    ComputeImagePosition(ratio, xoffset, yoffset);
    cr->translate(xoffset, yoffset);
    cr->scale(ratio, ratio);

    DrawView(cr);

    cr->restore();

    return true;
}

void App::ComputeImagePosition(double &ratio, double &xoffset, double &yoffset) {
    double image_width = image_->get_width();
    double image_height = image_->get_height();

    double area_width = drawing_area_->get_allocated_width();
    double area_height = drawing_area_->get_allocated_height();

    double width_ratio = area_width / image_width;
    double height_ratio = area_height / image_height;

    ratio = std::fmin(width_ratio, height_ratio);

    double extra_width = area_width - (image_width * ratio);
    double extra_height = area_height - (image_height * ratio);

    xoffset = (extra_width / 2);
    yoffset = (extra_height / 2);
}

void App::DrawView(const Cairo::RefPtr<Cairo::Context> &cr) {
    cr->save();

    Cairo::RefPtr<Cairo::ImageSurface> surface = Gdk::Cairo::create_surface_from_pixbuf(image_, 1);
    cr->set_source(surface, 0, 0);
    cr->paint();

    // Draw the circle in red
    cr->set_source_rgb(1, 0, 0);
    circle_.Draw(cr);

    // Draw the line in green
    cr->set_source_rgb(0, 1, 0);
    line_.Draw(cr);

    cr->restore();
}

void App::FindNeedle() {
    if (circle_.r <= 0)
        return;

    assert(image_->get_colorspace() == Gdk::Colorspace::COLORSPACE_RGB);
    assert(image_->get_n_channels() == 3);

    const int width = image_->get_width();
    const int height = image_->get_height();
    const int row_stride = image_->get_rowstride();
    const guint8 *data = image_->get_pixels(); // TODO do we need to free?

    double max_redness = NAN;
    double max_redness_angle = NAN;

    // Iterate angles
    for (int t = 0; t < NUM_ANGLES; t++) {
        double angle = (2 * M_PI) * ((double)t / NUM_ANGLES);
        double cosa = std::cos(angle);
        double sina = std::sin(angle);

        double sum = 0;
        // Iterate radii
        for (int d = inner_ * circle_.r; d < outer_ * circle_.r; d++) {
            int x = circle_.x + (d * cosa);
            int y = circle_.y + (d * sina);

            if (x >= 0 && x < width && y >= 0 && y < height) {
                int index = (row_stride * y) + (x * 3);

                int r = data[index];
                int g = data[index + 1];
                int b = data[index + 2];
                
                // Redness Metric
                // If the pixel is black we'll end up with a NaN value which we
                // would rather ignore than allow it to affect the entire angle
                if (r != 0 || g != 0 || b != 0)
                    // Include the red component in the denominator to cancel
                    // out pixels that only have a red component (so they don't
                    // go off to infinity and instead just go to 3 (which is
                    // still pretty high)...
                    sum += (double)(3 * r) / (double)(r + b + g);
            } else {
                break;
            }
        }

        double redness = sum / (double)circle_.r;

        if (save_hist_)
            hist_[t] = redness;

        if (std::isnan(max_redness) || redness > max_redness) {
            max_redness = redness;
            max_redness_angle = angle;
        }
    }

    if (std::isnan(max_redness))
        max_redness_angle = 0;

    angle_diff_ = AngleDiff(angle_, max_redness_angle);
    angle_ = max_redness_angle;

    // One full rotation of the dial is 10 gallons
    // TODO make sure this is accurate.
    reading_ += angle_diff_ * (10 / (2 * M_PI));

    double cos_needle = std::cos(max_redness_angle);
    double sin_needle = std::sin(max_redness_angle);

    line_.x1 = circle_.x + inner_ * (circle_.r * cos_needle);
    line_.y1 = circle_.y + inner_ * (circle_.r * sin_needle);

    line_.x2 = circle_.x + outer_ * (circle_.r * cos_needle);
    line_.y2 = circle_.y + outer_ * (circle_.r * sin_needle);
}

double App::AngleDiff(double a, double b) {
    // TODO make sure this is accurate.
    double diff1 = (b - a);
    double diff2 = diff1 + 2 * M_PI;
    double diff3 = diff1 - 2 * M_PI;

    double absdiff1 = std::fabs(diff1);
    double absdiff2 = std::fabs(diff2);
    double absdiff3 = std::fabs(diff3);

    if (absdiff1 <= absdiff2 && absdiff1 <= absdiff3)
        return diff1;
    else if (absdiff2 <= absdiff1 && absdiff2 <= absdiff3)
        return diff2;
    else // if (absdiff3 <= absdiff1 && absdiff3 <= absdiff2)
        return diff3;
}

void App::Output() {
    std::time_t now;
    std::time(&now);

    char date_string[20];
    std::strftime(date_string, 20, "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    std::FILE *out_file = std::fopen(DATA_FILE, "a");
    std::fprintf(out_file, "%s,%f\n", date_string, reading_);
    std::fclose(out_file);

    if (save_hist_) {
        std::FILE *hist_file = std::fopen(HIST_FILE, "a");
        std::fprintf(hist_file, "%s", date_string);
        for (int i = 0; i < NUM_ANGLES; i++)
            std::fprintf(hist_file, ",%f", hist_[i]);
        std::fprintf(hist_file, "\n");
        std::fclose(hist_file);
    }

    if (save_all_images_ || (save_images_ && std::fabs(prev_reading_ - reading_) >= 10)) {
        prev_reading_ = reading_;

        char file_name[64];
        mkdir(IMAGES_FOLDER, 0777); // TODO Not Portable
        std::strftime(file_name, 64, IMAGES_FOLDER "/%Y-%m-%dT%H_%M_%S.jpg", localtime(&now));
        
        image_->save(file_name, "jpeg");
    }

    if (save_debug_images_) {
        char file_name[64];
        mkdir(DEBUG_FOLDER, 0777); // TODO Not Portable
        strftime(file_name, 64, DEBUG_FOLDER "/%Y-%m-%dT%H_%M_%S.png", localtime(&now));

        int width = image_->get_width();
        int height = image_->get_height();

        Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(Cairo::Format::FORMAT_RGB24, width, height);
        Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);

        DrawView(cr);

        surface->flush();

        surface->write_to_png(file_name); // TODO less than ideal
    }
}

void App::AskForReading() {
    new_frame_.block();

    Gtk::Dialog dialog("Reading", true);
    dialog.add_button("_OK", Gtk::ResponseType::RESPONSE_OK);

    Gtk::Box *box = dialog.get_content_area(); // TODO do I have to free this?

    Gtk::Label label("Current reading of dial");
    box->add(label);

    Gtk::Entry entry;
    entry.set_text(std::to_string(reading_));
    box->add(entry);

    dialog.show_all();
    int response = dialog.run();

    if (response == Gtk::ResponseType::RESPONSE_OK) {
        reading_ = std::atof(entry.get_text().c_str());
    }

    new_frame_.unblock();
}

bool App::HandleKeyPress(GdkEventKey *event) {
    // We only want presses, not releases
    if (event->type != GDK_KEY_PRESS)
        return false;

    if (running_) {
        switch (event->keyval) {
        // Pause
        case GDK_KEY_space:
            running_ = false;
            
            output_timeout_.disconnect();
            ted_timeout_.disconnect();

            Refresh();

            return true;
        default:
            return false;
        }
    } else {
        switch (event->keyval) {
        // Play
        case GDK_KEY_space: {
            running_ = true;

            if (output_timeout_) {
                output_timeout_.disconnect();
            }
            output_timeout_ = Glib::signal_timeout().connect(sigc::mem_fun(this, &App::HandleOutputTimeout), OUTPUT_RATE);
            
            if (run_ted_) {
                if (ted_timeout_) {
                    ted_timeout_.disconnect();
                }
                output_timeout_ = Glib::signal_timeout().connect_seconds(sigc::mem_fun(this, &App::HandleTEDTimeout), 3600);
            }

            Output();
            Refresh();

            return true;
        }
        case GDK_KEY_j: {
            if ((event->state & GDK_CONTROL_MASK) != 0) {
                inner_ = std::fmin(outer_, inner_ + 0.1);
            } else {
                outer_ = std::fmin(1, outer_ + 0.1);
            }

            FindNeedle();
            Refresh();

            return true;
        }
        // Move needle detection down
        case GDK_KEY_k: {
            if ((event->state & GDK_CONTROL_MASK) != 0) {
                inner_ = std::fmax(0, inner_ - 0.1);
            } else {
                outer_ = std::fmax(inner_, outer_ - 0.1);
            }

            FindNeedle();
            Refresh();

            return true;
        }
        // Ask the user for the current reading
        case GDK_KEY_r: {
            AskForReading();
            Refresh();

            return true;
        }
        default: {
            return false;
        }
        }
    }
}

bool App::HandleButtonPress(GdkEventButton *event) {
    if (event->type != GDK_BUTTON_PRESS)
        return false;

    switch (event->button) {
    // left button pressed
    case 1:
        if (!running_) {
            double ratio, xoffset, yoffset;
            ComputeImagePosition(ratio, xoffset, yoffset);

            double x = ((double)event->x - xoffset) / ratio;
            double y = ((double)event->y - yoffset) / ratio;

            // start dragging the circle
            circle_.x = x;
            circle_.y = y;
            circle_.r = 0;

            line_.x1 = 0;
            line_.y1 = 0;
            line_.x2 = 0;
            line_.y2 = 0;

            // TODO Refresh here?
            Refresh();

            return true;
        }

        return false;
    default:
        return false;
    }
}

bool App::HandleMotion(GdkEventMotion *event) {
    if (!running_) {
        // left button is pressed; set new circle radius
        if ((event->state & GDK_BUTTON1_MASK) != 0) {
            double ratio, xoffset, yoffset;
            ComputeImagePosition(ratio, xoffset, yoffset);

            double x = ((double)event->x - xoffset) / ratio;
            double y = ((double)event->y - yoffset) / ratio;

            int dx = x - circle_.x;
            int dy = y - circle_.y;

            circle_.r = sqrt(dx * dx + dy * dy);

            // TODO find needle here?
            FindNeedle();
            Refresh();

            return true;
        }
    }

    return false;
}

bool App::HandleButtonRelease(GdkEventButton *event) {
    if (event->type != GDK_BUTTON_RELEASE)
        return false;

    switch (event->button) {
    // left button released
    case 1:
        if (!running_) {
            FindNeedle();
            Refresh();

            return true;
        }
        
        return false;
    default:
        return false;
    }
}

bool App::HandleOutputTimeout() {
    if (running_) {
        Output();
    }

    return running_;
}

bool App::HandleTEDTimeout() {
    if (running_ && run_ted_) {
        int result = system(TED_PATH " auto");

        if (result != 0) {
            std::cout << "Failed to execute TED" << std::endl;
        }
    }

    return running_;
}
