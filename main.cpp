
/******************************************************************************
 *
 * this is a development program which is intended to do ocr on the numbers in 
 * a water meter.
 * 
 * Written by Tom Mullins
 *
 ******************************************************************************
 */

#include "app.h"

// #include <signal.h>
// #include <sys/stat.h>
// #define main_cpp
// #include "app.h"
// #include "pipeline.h"

// #define TED_PATH "~/ted/hourly/datacollection.py"

// // number of angles around the circle to check average hue
// #define ANGLES 512

// /* === function declarations; comments accompany each function further down, by the definitions === */
// void makewindow();
// void cleanupwindow();
// void nextframe();
// void recalc();
// void redraw();
// void searchcircle();
// // these are all callback functions which control keyboard, mouse, and window drawing routines
// gboolean destroy(GtkWidget*, gpointer data);
// gboolean expose(GtkWidget*, GdkEventExpose*, gpointer);
// gboolean histexpose(GtkWidget*, GdkEventExpose*, gpointer);
// gboolean timeout(gpointer);
// gboolean teddata(gpointer);
// gboolean keypress(GtkWidget*, GdkEventKey*, gpointer);
// gboolean buttonpress(GtkWidget*, GdkEventButton*, gpointer);
// gboolean motion(GtkWidget*, GdkEventMotion*, gpointer);

// /* === global variables == */
// image mainimage;            // the main image, to be interpreted (the type image is defined as a struct in main.h)
// bool running = false;       // whether or not it is periodically updating and writing to a file
// bool saveimages = false;    // whether or not to save a frame every five seconds into images/ (set by -s)
// bool saveall = false;       // whether to save every image, or only after the reading has changed enough (set by -a)
// float lastSaveReading = 0;  // used with saveall == false to see how much the reading has changed
// int currentReading = 0;     // every time the needle goes backwards, this is increased by 10
// int needleMethod = 3;       // set by keys 1,2,3; describes start point of "needle" in searchcircle
// bool runted = false;        // whether to run the ted collection program every hour (set by -t)

// GtkWidget *window,      // main window
//     *drawingarea,       // place where mainpixbuf is drawn
//     *numlabel,          // displays guessed digit values to user
//     *hist;              // usually not used... uncomment the line below
// GdkPixbuf *mainpixbuf;  // used to display the main image
// GdkGC *red, *green;     // colors
// GMainLoop *mainloop;    // program's main loop, which will call above callback functions on events


// struct circle_t {
//     int x, y, r;
// } circle = {0};         // describes the circle to read the needle (later may be moved to its own class, like place and digit)
// float circlefrac = 0;   // current angle, 0-1, filled in searchcircle
// float prevcirclefrac = 0; // previous circlefrac
// struct cline_t {
//     int x1, y1, x2, y2;
// } circleline = {0};     // describes line within circle to be drawn, where the needle is thought to be

/*
 *  === main ===
 *
 *  first function run in program. argc contains the number of command line
 *  arguments, and argv is an array of argument strings
 */

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
                        g_print("Unrecognized option %s\n", argv[i][j]);
                        exit(1);
                }
            }
        }
    }

    // initialize GTK and GST
    gst_init(&argc, &argv);
    gtk_init(&argc, &argv);

    App *app = new App(runTED, saveImages, saveAll);
    app->Run();
    
    return 0;
}

// /*
//  *  === makewindow ===
//  *
//  *  creates GTK and GDK objects necessary for the main window
//  */

// void makewindow(Pipeline *pipeline) {
    
//     // create main window and attach keyboard and closure callbacks
//     window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

//     gtk_signal_connect(GTK_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);
//     gtk_signal_connect(GTK_OBJECT(window), "key-press-event", G_CALLBACK(keypress), NULL);
    
//     // create area to draw main image in, and attach mouse and drawing callbacks
//     drawingarea = gtk_drawing_area_new();
//     gtk_signal_connect(GTK_OBJECT(drawingarea), "expose-event", G_CALLBACK(expose), NULL);
//     gtk_signal_connect(GTK_OBJECT(drawingarea), "button-press-event", G_CALLBACK(buttonpress), NULL);
//     gtk_signal_connect(GTK_OBJECT(drawingarea), "button-release-event", G_CALLBACK(buttonpress), NULL);
//     gtk_signal_connect(GTK_OBJECT(drawingarea), "motion-notify-event", G_CALLBACK(motion), NULL);
//     gtk_widget_add_events(drawingarea, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK);
    
//     // create area to draw histogram in
//     hist = gtk_drawing_area_new();
//     gtk_widget_set_size_request(hist, 256, 256);
//     gtk_signal_connect(GTK_OBJECT(hist), "expose-event", G_CALLBACK(histexpose), NULL);
    
//     // create the label displaying the guessed values
//     numlabel = gtk_label_new("");
//     gtk_misc_set_alignment(GTK_MISC(numlabel), 0, 0);
    
//     // create a vbox, which displays a number of widgets one on top of the next, and pack everything into it
//     GtkWidget *vbox = gtk_vbox_new(FALSE, 2),
//         *hbox = gtk_hbox_new(FALSE, 2);
//     gtk_box_pack_start(GTK_BOX(vbox), drawingarea, FALSE, FALSE, 0);
//     gtk_box_pack_start(GTK_BOX(vbox), numlabel, FALSE, FALSE, 0);
//     gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
//     //gtk_box_pack_start(GTK_BOX(hbox), hist, FALSE, FALSE, 0);
//     gtk_container_add(GTK_CONTAINER(window), hbox);
    
//     // create the pixbuf to display the main image in
//     /*mainpixbuf = gdk_pixbuf_new_from_data((guchar*) mainimage.data, GDK_COLORSPACE_RGB, false, 8,
//         mainimage.width, mainimage.height, mainimage.width*3, NULL, NULL);*/
    
//     // show the window and its children
//     gtk_widget_show_all(window);
    
//     // create red and green, for the boxes
//     GdkColor c;
//     c.red = 65535/3*2;
//     c.blue = c.green = 0;
//     red = gdk_gc_new(GDK_DRAWABLE(window->window));
//     gdk_gc_set_rgb_fg_color(red, &c);
//     c.green = 65535/3*2;
//     c.blue = c.red = 0;
//     green = gdk_gc_new(GDK_DRAWABLE(window->window));
//     gdk_gc_set_rgb_fg_color(green, &c);
// }

// /*
//  *  === cleanupwindow ===
//  *
//  *  free objects made in makewindow
//  */

// void cleanupwindow() {
//     g_object_unref(G_OBJECT(red));
//     g_object_unref(G_OBJECT(green));
// }

// /*
//  *  === nextframe ===
//  *
//  *  goes to the next frame and updates everything
//  */

// void nextframe() {
//     capture(&mainpixbuf, &mainimage);
//     recalc();
// }

// /*
//  *  === recalc ===
//  *
//  *  updates everything
//  */

// void recalc() {
//     searchcircle();
// }

// /*
//  *  === redraw ===
//  *
//  *  refills numlabel, with the guessed value from each digit, and then tells
//  *  the window to redraw the main image
//  */

// void redraw() {
//     char buf[128];
//     /*int sum = 0;
//     for (int i = numplaces-1, mult = 10; i >= 0; i--, mult *= 10)
//         sum += places[i].getvalue()*mult;
//     float value = sum + circlefrac*10;*/
//     if (circlefrac-prevcirclefrac < -.4)
//         currentReading += 10;
//     prevcirclefrac = circlefrac;
//     float value = currentReading + circlefrac*10;
//     sprintf(buf, "%s\t\t%.2f\t\tThreshold offset: %d\t\t",
//         running? "running" : "=== PAUSED ===", value, thresholdoffset);
//     gtk_label_set_text(GTK_LABEL(numlabel), buf);
//     if (running) {
//         char datestr[64];
//         time_t now;
//         time(&now);
//         strftime(datestr, 64, "%Y-%m-%d %H:%M:%S", localtime(&now));
//         FILE *outfile = fopen("meter.csv", "a");
//         fprintf(outfile, "%s,%f\n", datestr, value);
//         fclose(outfile);
//         if (saveimages && (saveall || fabs(lastSaveReading-value) > 1)) {
//             mkdir("images", 0777);
//             strftime(datestr, 64, "images/%Y-%m-%d %H_%M_%S.ppm", localtime(&now));
//             savefile(datestr, &mainimage);
//             lastSaveReading = value;
//         }
//     }
//     gdk_window_invalidate_rect(drawingarea->window, NULL, FALSE);
// }

// /*
//  *  === searchcircle ===
//  *
//  *  checks lines from the center of circle to the edges at many angles, looking
//  *  for the average hue closest to red, to find the needle, and fills
//  *  circleline
//  */

// void searchcircle() {
    
//     if (circle.r < 1) circle.r = 1;
    
//     //int size = mainimage.width*mainimage.height*3;
//     byte *oldimage = mainimage.data;//(byte*) malloc(size);
//     //memcpy(oldimage, mainimage.data, size);
    
//     // check every angle for best hue
//     int min = ANGLES*2;
//     float mina = 0, minsin = 0, mincos = 0;
//     int start = (needleMethod == 1)? 0 : (needleMethod == 2)? -circle.r*1/6 : circle.r*4/5;
//     for (int i = 0; i < ANGLES; i++) {
//         float a = M_PI*2*i/ANGLES,
//             cosa = cos(a),
//             sina = sin(a);
        
//         // find the sum of the pixels' proximity to red along radius at angle i
//         int sum = 0;
//         for (int j = start; j < circle.r; j++) {
//             int index = (int(circle.x+j*cosa) + int(circle.y + j*sina)*mainimage.width)*3,
//                 r = 255-oldimage[index],
//                 g = oldimage[index+1],
//                 b = oldimage[index+2];
            
//             /*mainimage.data[index] = mainimage.data[index+1] = mainimage.data[index+2] =
//                 sqrt((r*r+g*g+b*b)/3);*/
            
//             // this would be proximity to black, except r = 255-r
//             sum += (2*r+b+g)/4;//sqrt(r*r+g*g+b*b); //180*(atan2(sqrt(3)*(g-b)/2, r-(g+b)/2)/M_PI + 1);
            
//         }
        
//         // check if it's closer to 180 than previous
//         int d = sum/circle.r;
//         histdata[i/2] = d;
//         if (d < min) {
//             min = d;
//             mina = a;
//             minsin = sina;
//             mincos = cosa;
//         }
        
//     }
    
//     //free(oldimage);
    
//     // set circleline to show the best angle and redraw
//     circleline.x1 = circle.x+start*mincos;
//     circleline.y1 = circle.y+start*minsin;
//     circleline.x2 = circle.x+circle.r*mincos;
//     circleline.y2 = circle.y+circle.r*minsin;
//     circlefrac = mina/M_PI/2 + .25;
//     if (circlefrac >= 1) circlefrac -= 1;
//     redraw();
//     if (hist->window) gdk_window_invalidate_rect(hist->window, NULL, FALSE);
    
// }

// /*
//  *  below are GTK callbacks for events, like key presses or mouse movement
//  */

// // called when a key is pressed
// gboolean keypress(GtkWidget *widget, GdkEventKey *event, gpointer data) {
//     if (event->type != GDK_KEY_PRESS) return FALSE;
    
//     if (running) {
//         if (event->keyval == GDK_space)
//             running = false;
//         else return FALSE;
//     } else {
//        switch (event->keyval) {
            
//             case GDK_space:
//                 nextframe();
//                 break;
            
//             case GDK_Right:
//                 nextframe();
//                 break;
            
//             case GDK_Left:
//                 skipback();
//                 nextframe();
//                 break;
                      
//             case GDK_p:
//                 running = true;
//                 timeout(NULL);
//                 g_timeout_add_seconds(5, timeout, NULL);
//                 if (runted) {
//                     teddata(NULL);
//                     g_timeout_add_seconds(3600, teddata, NULL);
//                 }
//                 redraw();
//                 break;
            
//             case GDK_j:
//                 thresholdoffset++;
//                 recalc();
//                 break;
            
//             case GDK_k:
//                 thresholdoffset--;
//                 recalc();
//                 break;
            
//             case GDK_r: {
//                 GtkWidget *dialog = gtk_dialog_new_with_buttons("Reading",
//                     GTK_WINDOW(window), GTK_DIALOG_MODAL,
//                     GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
//                 GtkWidget *spin = gtk_spin_button_new_with_range(0, 999999, 1);
//                 GtkWidget *label = gtk_label_new("Current reading of movable digits only:");
//                 gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, FALSE, 2);
//                 gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), spin, TRUE, FALSE, 2);
//                 gtk_widget_show(spin);
//                 gtk_widget_show(label);
//                 gtk_dialog_run(GTK_DIALOG(dialog));
//                 gtk_spin_button_update(GTK_SPIN_BUTTON(spin));
//                 currentReading = 10*gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
//                 gtk_widget_destroy(dialog);
//                 redraw();
//                 break;
//             }
            
//             default: return FALSE;
//         }
        
//     }
    
//     return TRUE;
// }

// // called every 5 seconds
// gboolean timeout(gpointer data) {
//     if (running) nextframe();
//     return running;
// }

// // called every hour
// gboolean teddata(gpointer data) {
//     if (running) int unused = system(TED_PATH " auto");
//     return running;
// }

// // called when a mouse button is pressed or released
// gboolean buttonpress(GtkWidget *widget, GdkEventButton *event, gpointer data) {
//     switch (event->type) {
//         case GDK_BUTTON_PRESS:
//             switch (event->button) {
                
//                 // left button pressed
//                 case 1:
//                     if (!running) {
                        
//                         // start dragging the circle
//                         circle.x = event->x;
//                         circle.y = event->y;
                        
//                         // reset circleline until we're finished dragging
//                         memset(&circleline, 0, sizeof circleline);
                        
//                     }
//                     break;

//                 default: return FALSE;
//             }
//             break;
//         case GDK_BUTTON_RELEASE:
//             switch (event->button) {
                
//                 // left button released
//                 case 1:
//                     if (!running) searchcircle();
//                     break;
                
//                 default: return FALSE;
//             }
//             break;
//         default: return FALSE;
//     }
//     return TRUE;
// }

// // called when the mouse is moved while a button is pressed
// gboolean motion(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
//     if (!running) {
        
//         // left button is pressed; set new circle radius
//         if (event->state & GDK_BUTTON1_MASK) {
//             int dx = event->x-circle.x, dy = event->y-circle.y;
//             circle.r = sqrt(dx*dx+dy*dy);
//             if (circle.r > circle.x)
//                 circle.r = circle.x;
//             if (circle.r > mainimage.width-circle.x-1)
//                 circle.r = mainimage.width-circle.x-1;
//             if (circle.r > circle.y)
//                 circle.r = circle.y;
//             if (circle.r > mainimage.height-circle.y-1)
//                 circle.r = mainimage.height-circle.y-1;
//             redraw();
//             return TRUE;
//         }
//     }
//     return FALSE;
// }

// // called when the window is closed
// gboolean destroy(GtkWidget *widget, gpointer data) {
//     quit();
//     return FALSE;
// }

// void quit() {
//     g_main_loop_quit(mainloop);
// }

// // called whenever we should redraw the contents of drawingarea
// gboolean expose(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
    
//     // draw main image
//     gdk_draw_pixbuf(event->window, NULL, mainpixbuf, 0, 0, 0, 0,
//         -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

//     // draw the circle and its line
//     gdk_draw_line(event->window, green, circleline.x1, circleline.y1, circleline.x2, circleline.y2);
//     gdk_draw_arc(event->window, red, FALSE, circle.x-circle.r, circle.y-circle.r, circle.r*2, circle.r*2, 0, 360*64);
    
//     return TRUE;
// }

// // called whenever we should redraw the histogram
// gboolean histexpose(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
    
//     // get height of area, so we can align with bottom
//     int h;
//     gdk_drawable_get_size(event->window, NULL, &h);
    
//     // draw background
//     gdk_draw_rectangle(event->window, widget->style->white_gc, TRUE, 0, 0, 256, h);
    
//     // line at threshold value (removed, since it's no longer global but per-place)
//     //gdk_draw_line(event->window, red, threshold, 0, threshold, h);
    
//     // line for each pixel value, for nice bar graph
//     for (int i = 0; i < 256; i++)
//         gdk_draw_line(event->window, widget->style->black_gc, i, h-histdata[i], i, h);
    
//     return TRUE;
// }
