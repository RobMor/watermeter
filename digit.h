#include "main.h"

// number of pixels within which mouse must be for drag to succeed
#define SNAP_SIZE 5

// minimum width - 1 for a gap or number space
#define GAP_SMOOTH 1

// if defined, will use tesseract-ocr instead of own ppm cmpimages
#define USE_TESSERACT

const char tessdata[] = "/usr/share/tessdata";

// This macro was causing problems on my system and was only used twice
// #define swap(a, b) do {int temp = a; a = b; b = temp;} while (0)

typedef struct rect_t {
    int x1, y1, x2, y2;
} rect;

// specifies corner or edge which is being dragged
typedef enum loc_t {
    none, tl, tr, bl, br, t, l, b, r
} location;

// specifies how a digit should be compared; if the whole digit is there, it
// will be stretch, while if only part is there, it will be top or bottom
typedef enum algn_t {top, bottom, stretch} alignment;


/*
 *  === class digit ===
 *
 *  represents one digit; displayed as a green box
 */

class digit {
    private:
        static image cmpimages[10]; // images to which ownimage is compared to guess its value
        rect bounds;            // rectangular bounds, relative to MAIN image (not place's bounds)
        image ownimage;         // local copy of place's thresholdimage limited to bounds
    public:
        static void init();
        static void cleanup();
        void draw(GdkWindow*, GdkGC*, GdkGC*);
        void setbounds(int, int, int, int);
        void update(image*, rect*);
        void compare(byte[], alignment);
        void saveto(const char*);
        ~digit();
};


/*
 *  === class place ===
 *
 *  represents a place where a digit may be; can have two digits if the
 *  value is halfway in between two numbers
 */

class place {
    private:
        rect bounds;            // rectangular bounds, relative to main image
        location dragging;      // which corner or edge is being dragged, or "none"
        int threshold;          // pixel intensity separating white pixels from black ones in thresholdimage
        image thresholdimage;   // purely black and white image
        GdkPixbuf *pixbuf;      // used to display thresholdimage
        digit digits[2];        // child digits
        int numdigits;          // number of digits valid in above array
        int value;              // suspected value of this place, filled in update
        char valuestr[16];      // returned by tesseract, filled in update
        int probabilities[10];  // persistant between frames, until value changes
        bool *isgap;
    public:
        void draw(GdkWindow*, GdkGC*, GdkGC*);
        void update(image*);
        void forcestartdrag(int, int);
        int startdrag(int, int);
        void drag(int, int, int, int);
        void enddrag(int, int);
        bool scanbounds(FILE*);
        void printbounds(FILE*);
        int getvalue() {return value;}
        char* getvaluestr() {return valuestr;}
        void saveto(const char*);
        place();
        ~place();
};
