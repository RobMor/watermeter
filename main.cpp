#include "app.h"

int main(int argc, char *argv[]) {
    bool saveImages = false;
    bool saveAll = false;
    bool runTED = false;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 's':
                        saveImages = true;
                        break;
                    case 'a':
                        saveAll = true;
                        break;
                    case 't':
                        runTED = true;
                        break;
                    default:
                        g_print("Unrecognized option %c\n", argv[i][j]);
                        exit(1);
                }
            }
        }
    }

    // initialize GTK and GST
    gst_init(&argc, &argv);

    App *app = new App(runTED, saveImages, saveAll);
    app->Run();
    
    return 0;
}
