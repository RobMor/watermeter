#include "wm-app.h"

Glib::RefPtr<WmApp> WmApp::Create() {
    WmApp *app = new WmApp();

    app->signal_command_line().connect(sigc::mem_fun(app, WmApp::HandleCommandLine));
    app->signal_activate().connect(sigc::mem_fun(app, WmApp::HandleActivate));

    return Glib::RefPtr<WmApp>(app);
}

int WmApp::HandleCommandLine(Glib::RefPtr<Gio::ApplicationCommandLine> &cmd) {
    Glib::OptionContext ctx;

    Glib::OptionGroup main_group("options", "options for general use");

    Glib::OptionEntry save_images_option;
    save_images_option.set_short_name('s');
    save_images_option.set_description("Save the current frame every full rotation of the dial.");
    main_group.add_entry(save_images_option, save_images_);

    Glib::OptionEntry save_all_images_option;
    save_all_images_option.set_short_name('a');
    save_all_images_option.set_description("Save every frame.");
    main_group.add_entry(save_all_images_option, save_all_images_);

    Glib::OptionEntry run_ted_option;
    run_ted_option.set_short_name('t');
    run_ted_option.set_description("Run the TED data collection program every hour.");
    main_group.add_entry(run_ted_option, run_ted_);

    Glib::OptionGroup debug_group("debug", "options for use in debugging");

    Glib::OptionEntry save_debug_images_option;
    save_debug_images_option.set_short_name('d');
    save_debug_images_option.set_description("Save debug images every frame.");
    debug_group.add_entry(save_debug_images_option, save_debug_);

    Glib::OptionEntry save_hist_option;
    save_hist_option.set_short_name('h');
    save_hist_option.set_description("Save a histogram of needle detection data every frame.");
    debug_group.add_entry(save_hist_option, save_hist_);

    return 0;
}

void WmApp::HandleActivate() {
    return;
}