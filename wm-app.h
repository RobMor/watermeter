#ifndef __WM_APP_H_
#define __WM_APP_H_

#include <gtkmm.h>

class WmApp : public Gtk::Application
{
public:
    static Glib::RefPtr<WmApp> Create();

protected:
    WmApp() : Gtk::Application("wm.app", Gio::ApplicationFlags::APPLICATION_HANDLES_COMMAND_LINE) {};

    int HandleCommandLine(Glib::RefPtr<Gio::ApplicationCommandLine> &cmd);
    void HandleActivate() override;

private:
    bool save_images_;
    bool save_all_images_;
    bool run_ted_;

    bool save_debug_;
    bool save_hist_;
};

#endif
