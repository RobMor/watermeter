#include <iostream>

#include "app.h"

int main(int argc, char *argv[]) {
    Glib::RefPtr<App> app = App::Create();

    return app->run(argc, argv);
}
