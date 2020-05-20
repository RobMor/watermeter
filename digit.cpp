#include "digit.h"
#include "ppm.h"
#include <csignal>

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

image digit::cmpimages[10];
tesseract::TessBaseAPI *ocr;

/*
 *  === digit::init ===
 *
 *  this is static, and should be called once at the beginning of the
 *  program to load all of the necessary comparison images. If any fail to
 *  load, an error message will still be printed but otherwise it will pass
 *  quietly
 */

void digit::init() {
 ocr = new tesseract::TessBaseAPI();
#ifdef USE_TESSERACT
    if (!ocr->SetVariable("tessedit_char_whitelist", "0123456789"))
        printf("TessBaseAPI::SetVariable name lookup failed\n");
	ocr->Init(tessdata, NULL);
#else
    for (int i = 0; i < 10; i++) {
        char buf[12];
        sprintf(buf, "%d.ppm", i);
        if (loadfile(buf, &cmpimages[i])) {
            free(cmpimages[i].data);
            cmpimages[i].data = NULL;
        }
    }
#endif
}

/*
 *  === digit::cleanup ===
 *
 *  this is also static, and should be called at the end of the program
 */

void digit::cleanup() {
#ifdef USE_TESSERACT
    ocr->End();
#else
    for (int i = 0; i < 10; i++)
        free(cmpimages[i].data);
#endif
}

/*
 *  === digit::draw ===
 *
 *  draw the green rectangle onto window
 */

void digit::draw(GdkWindow *window, GdkGC *red, GdkGC *green) {
#ifndef USE_TESSERACT
    gdk_draw_rectangle(window, green, FALSE, bounds.x1-1, bounds.y1-1, bounds.x2-bounds.x1+1, bounds.y2-bounds.y1+1);
#endif
}

/*
 *  === digit::setbounds ===
 *
 *  sets the bounds while being resized; after the resizing is finished,
 *  update must be called to make everything reflect the new bounds
 */

void digit::setbounds(int x1, int y1, int x2, int y2) {
    bounds.x1 = x1;
    bounds.y1 = y1;
    bounds.x2 = x2;
    bounds.y2 = y2;
}

/*
 *  === digit::update ===
 *
 *  squishes vertical edges of bounds inwards to strip off any white space,
 *  restricting its search to placebounds, and then refills ownimage with
 *  data from placeimage
 */

void digit::update(image *placeimage, rect *placebounds) {
#ifndef USE_TESSERACT
    int max = bounds.x2-bounds.x1,
        mid = max/2;
    
    // start at the middle, and go back towards the left, looking for the first white column
    for (int x = mid; x >= 0; x--) {
        int y;
        for (y = bounds.y1-placebounds->y1; y < bounds.y2-placebounds->y1; y++)
            if (placeimage->data[(y*placeimage->width+x)*3] == 0) break;
        if (y == bounds.y2-placebounds->y1) {
            bounds.x1 = x+placebounds->x1;
            break;
        }
    }
    
    // start at the middle, and go forward towards the right, again looking for the first white column
    for (int x = mid+1; x < max; x++) {
        int y;
        for (y = bounds.y1-placebounds->y1; y < bounds.y2-placebounds->y1; y++)
            if (placeimage->data[(y*placeimage->width+x)*3] == 0) break;
        if (y == bounds.y2-placebounds->y1) {
            bounds.x2 = x+placebounds->x1;
            break;
        }
    }
    
    // update image's width and height
    ownimage.width = bounds.x2-bounds.x1;
    ownimage.height = bounds.y2-bounds.y1;
    
    // reallocate image
    free(ownimage.data);
    ownimage.data = (byte*) malloc(ownimage.width*ownimage.height*3);
    
    // copy image from parent place's thresholdimage
    for (int y = 0; y < ownimage.height; y++)
        memcpy(ownimage.data+y*ownimage.width*3, placeimage->data+((bounds.y1-placebounds->y1+y)*placeimage->width+bounds.x1-placebounds->x1)*3, ownimage.width*3);
#endif
}

/*
 *  === digit::compare ===
 *
 *  fills ret, which should have 10 elements, with values 0-255. The more a
 *  cmpimage matched ownimage, the higher the ret value. align should be
 *  "stretch" if the entire number is available, "top" if only the top half
 *  is available, and "bottom" if only the bottom half is available
 */

struct dump_t {
    int x, y, cmpx, cmpy;
    int ownwidth, ownheight, cmpwidth, cmpheight;
} dump;

void dumpimageinfo(int param) {
    printf("Segmentation Fault\n");
    printf("%d, %d, %d, %d\n", dump.x, dump.y, dump.cmpx, dump.cmpy);
    printf("%d, %d, %d, %d\n", dump.ownwidth, dump.ownheight, dump.cmpwidth, dump.cmpheight);
    exit(1);
}

void digit::compare(byte ret[], alignment align) {
#ifndef USE_TESSERACT
    
    // check every cmpimage against ownimage
    for (int i = 0; i < 10; i++)
        
        // only if the image exists
        if (cmpimages[i].data) {
            
            // number of pixels in ownimage which match cmpimage
            int matches = 0;
            
            for (int y = 0; y < ownimage.height; y++)
                for (int x = 0; x < ownimage.width; x++) {
                    
                    // since ownimage and cmpimage may be different sizes, we scale x by cmpimage's width over ownimage's width
                    // if align is "top" or "bottom", we scale y by that same amount, maintaining aspect ratio, and offset it to check only the top or only the bottom
                    // if align is "stretch", we simply scale y by cmpimage's height over ownimag's height
                    int cmpy;
                    switch (align) {
                        case bottom:
                            cmpy = (y-ownimage.height)*cmpimages[i].width/ownimage.width+cmpimages[i].height;
                            if (cmpy < 0) cmpy = 0;
                            break;
                        case top:
                            cmpy = y*cmpimages[i].width/ownimage.width;
                            break;
                        case stretch:
                            cmpy = y*cmpimages[i].height/ownimage.height;
                            break;
                        default:
                            printf("Warning: invalid align in digit::compare\n");
                    }
                    int cmpx = x*cmpimages[i].width/ownimage.width;
                    
                    dump.x = x;
                    dump.y = y;
                    dump.cmpx = cmpx;
                    dump.cmpy = cmpy;
                    dump.ownwidth = ownimage.width;
                    dump.ownheight = ownimage.height;
                    dump.cmpwidth = cmpimages[i].width;
                    dump.cmpheight = cmpimages[i].height;
                    signal(SIGSEGV, dumpimageinfo);
                    
                    // check if x, y matches scaled x, scaled y, and if so increment matches
                    if (cmpy < cmpimages[i].height && cmpx < cmpimages[i].width
                        && ownimage.data[(y*ownimage.width+x)*3]
                        == cmpimages[i].data[(cmpy*cmpimages[i].width+cmpx)*3])
                        matches++;
                    
                    signal(SIGSEGV, SIG_DFL);
                    
                }
            
            // return matches / total # pixels scaled to 0-255
            ret[i] = matches*256/(ownimage.width*ownimage.height+1);
            
        }
        
        // if cmpimages[i].data is NULL, just send back a 0% match
        else ret[i] = 0;
    
#endif
}

/*
 *  === digit::saveto ===
 *
 *  saves ownimage to a ppm file "name"
 */

void digit::saveto(const char *name) {
#ifndef USE_TESSERACT
    savefile(name, &ownimage);
#endif
}

/*
 *  === digit::~digit ===
 *
 *  this is the destructor, called whenever an instance goes out of scope
 */

digit::~digit() {
    free(ownimage.data);
}

/*
 *  === place::draw ===
 *
 *  draws the pixbuf, the red rectangle, and red lines over any gap rows to
 *  window; also called draw functions of child digits
 */

void place::draw(GdkWindow *window, GdkGC *red, GdkGC *green) {
    
    // draw red rectangle
    gdk_draw_rectangle(window, red, FALSE, bounds.x1-1, bounds.y1-1, bounds.x2-bounds.x1+1, bounds.y2-bounds.y1+1);
    
#ifndef USE_TESSERACT    
    // if we're currently being resized, don't draw thresholdimage, green boxes, or gap lines
    if (dragging == none) {
        
        // draw thresholdimage
        gdk_draw_pixbuf(window, NULL, pixbuf, 0, 0, bounds.x1, bounds.y1,
            -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
        
        // draw a line over every row with only white pixels
        for (int y = 0; y < thresholdimage.height; y++)
            if (isgap[y])
                gdk_draw_line(window, red, bounds.x1, bounds.y1+y, bounds.x2, bounds.y1+y);
        
        // call draw functions of child digits
        for (int i = 0; i < numdigits; i++)
            digits[i].draw(window, red, green);
    }
#endif

}

/*
 *  === freepixbuf ===
 *
 *  passed as the callback function to be called by GDK whenever a pixbuf is
 *  freed, so we can free its image data
 */

void freepixbuf(guchar *pixels, gpointer data) {
    free(pixels);
}

/*
 *  === place::update ===
 *
 *  do... everything
 */

void place::update(image *mainimage) {
#ifdef USE_TESSERACT
    char* newvaluestr = ocr->TesseractRect(mainimage->data, 3, mainimage->width*3,
        bounds.x1, bounds.y1, bounds.x2-bounds.x1, bounds.y2-bounds.y1);
    int newvalue;
    if (newvaluestr == NULL) {
        newvalue = 0;
        valuestr[0] = 0;
    } else {
        int j = 0;
        for (int i = 0; newvaluestr[i] && j < 15; i++) {
            if (newvaluestr[i] != '\n')
                valuestr[j++] = newvaluestr[i];
        }
        valuestr[j] = 0;
        delete[] newvaluestr;
        newvalue = valuestr[0] - 0x30;
        if (newvalue < 0 || newvalue > 9)
            newvalue = 0;
    }
    if (newvalue != value) {
        probabilities[newvalue] += 10;
        if (value == -1 || probabilities[newvalue] > probabilities[value]) {
            value = newvalue;
            memset(probabilities, 0, sizeof(probabilities));
            probabilities[value] = 25;
            probabilities[(value+1)%10] = 18;
        }
    }
#else
    // refill width/height
    thresholdimage.width = bounds.x2-bounds.x1;
    thresholdimage.height = bounds.y2-bounds.y1;
    
    // reallocate data, isgap, and pixbuf
    if (GDK_IS_PIXBUF(pixbuf)) gdk_pixbuf_unref(pixbuf);
    free(isgap);
    isgap = (bool*) malloc(thresholdimage.height * sizeof(bool));
    thresholdimage.data = (byte*) malloc(thresholdimage.width*thresholdimage.height*3);
    pixbuf = gdk_pixbuf_new_from_data((guchar*) thresholdimage.data, GDK_COLORSPACE_RGB, false, 8,
        thresholdimage.width, thresholdimage.height, thresholdimage.width*3, freepixbuf, NULL);
    
    // fill histogram data
    memset(histdata, 0, sizeof histdata);
    for (int y = 0; y < thresholdimage.height; y++)
        for (int x = 0; x < thresholdimage.width; x++) {
            int i = ((y+bounds.y1)*mainimage->width+x+bounds.x1)*3;
            histdata[(mainimage->data[i] + mainimage->data[i+1] + mainimage->data[i+2])/3]++;
        }
    
    // find the pixel value with the most pixels, and set the threshold to that minus thresholdoffset
    // (we want to ignore 0 and 255 because those tend to have a lot of values)
    int max = 1;
    for (int i = 2; i < 255; i++)
        if (histdata[i] > histdata[max])
            max = i;
    threshold = max + thresholdoffset;
    
    // refill thresholdimage from mainimage using new threshold and new bounds
    for (int y = 0; y < thresholdimage.height; y++) {
        
        // assume it's entirely white
        //isgap[y] = true;
        int num = 0;
        
        for (int x = 0; x < thresholdimage.width; x++) {
            
            // offsets in pixel arrays
            int sm = (y*thresholdimage.width+x)*3, lg = ((y+bounds.y1)*mainimage->width+x+bounds.x1)*3;
            
            // set to black or white
            thresholdimage.data[sm] = thresholdimage.data[sm+1] = thresholdimage.data[sm+2] =
                (mainimage->data[lg] + mainimage->data[lg+1] + mainimage->data[lg+2] >= threshold*3)? 255 : 0;
            
            // if we find any that aren't white, it's not a gap
            if (thresholdimage.data[sm] == 0)
                //isgap[y] = false;
                num++;
            
        }
        
        isgap[y] = num < thresholdimage.width/10;
        
    }
    
    // check isgap array for any gaps which are too thin
    int i;
    for (i = 0; i < thresholdimage.height-1 && isgap[i] == isgap[i+1]; i++);
    int dist = i+1;
    if (0 < dist && dist <= GAP_SMOOTH)
        for (int k = 0; k <= i; k++)
            isgap[k] = isgap[i];
    while (i < thresholdimage.height-1) {
        for (; i < thresholdimage.height-1 && isgap[i] == isgap[i+1]; i++);
        int j;
        for (j = i+1; j < thresholdimage.height && isgap[j] != isgap[i]; j++);
        dist = j-i-1;
        if (0 < dist && dist <= GAP_SMOOTH)
            for (int k = i+1; k < j; k++)
                isgap[k] = isgap[i];
        i = j-1;
    }
    
    // find digits
    int prevgapend = 0;
    numdigits = 0;
    for (int y = 0; y < thresholdimage.height; y++) {
        if (isgap[y]) {
            int gapstart = bounds.y1+y;
            for (y++; y < thresholdimage.height && isgap[y]; y++);
            int gapend = bounds.y1+y;
            switch (numdigits) {
                
                // first gap, assume there are two digits
                case 0:
                    numdigits = 2;
                    digits[0].setbounds(bounds.x1, bounds.y1, bounds.x2, gapstart);
                    digits[1].setbounds(bounds.x1, gapend, bounds.x2, bounds.y2);
                    break;
                
                // second gap, assume there's only one digit
                case 2:
                    numdigits = 1;
                    digits[0].setbounds(bounds.x1, prevgapend, bounds.x2, gapstart);
                    break;
            }
            prevgapend = gapend;
        }
    }
    
    // no gaps at all, assume there's one digit
    if (numdigits == 0) {
        numdigits = 1;
        digits[0].setbounds(bounds.x1, bounds.y1, bounds.x2, bounds.y2);
    }
    
    // update digits' images
    for (int i = 0; i < numdigits; i++)
        digits[i].update(&thresholdimage, &bounds);
    
    // compare digits
    byte probabilities[2][10];
    for (int i = 0; i < numdigits; i++)
        digits[i].compare(probabilities[i], (numdigits == 1)? stretch : i? top : bottom);
    
    // find maximum probability, or if there are two digits, the maximum sum between two consecutive digits
    max = 0;
    if (numdigits == 1) {
        for (int i = 1; i < 10; i++)
            if (probabilities[0][i] > probabilities[0][max])
                max = i;
    } else if (numdigits == 2) {
        for (int i = 1; i < 10; i++)
            if (probabilities[0][i] + probabilities[1][(i == 9)? 0 : i+1] >
                probabilities[0][max] + probabilities[1][max+1])
                max = i;
    }
    value = max;
#endif
}

/*
 *  === place::forcestartdrag ===
 *
 *  effectively creates a new place, with its corner at an absolute position
 */

void place::forcestartdrag(int x, int y) {
    bounds.x2 = bounds.x1 = x;
    bounds.y2 = bounds.y1 = y;
    startdrag(x, y);
}

/*
 *  === place::startdrag ===
 *
 *  returns nonzero if the mouse is close enough to an edge or corner, and
 *  subsequently drag should be called after any movement and enddrag should
 *  be called after the mouse is released. Returns 0 otherwise, and neither
 *  drag nor enddrag should subsequently be called
 */

int place::startdrag(int x, int y) {
    
    // fill boolean variable with information about whether the mouse is "near" a certain coordinate
    bool x1 = abs(x - bounds.x1) < SNAP_SIZE,
        x2 = abs(x - bounds.x2) < SNAP_SIZE,
        y1 = abs(y - bounds.y1) < SNAP_SIZE,
        y2 = abs(y - bounds.y2) < SNAP_SIZE;
    
    // more booleans, about whether it's within the box or not
    bool inx = bounds.x1 < x && x < bounds.x2,
        iny = bounds.y1 < y && y < bounds.y2;
    
    if (x1) {
        if (y1) dragging = tl;
        else if (y2) dragging = bl;
        else if (iny) dragging = l;
        else return 0;
    } else if (x2) {
        if (y1) dragging = tr;
        else if (y2) dragging = br;
        else if (iny) dragging = r;
        else return 0;
    } else if (y1 && inx) dragging = t;
    else if (y2 && inx) dragging = b;
    else return 0;
    return 1;
}

/*
 *  === place::drag ===
 *
 *  set new coordinates for the corner or edge being dragged, with what
 *  seems like far too much redundancy. If one edge is dragged past another,
 *  this will switch which edge is being dragged, forcing two things to
 *  always be true: x1 <= x2 && y1 <= y2
 */

void place::drag(int x, int y, int maxx, int maxy) {
    
    // set the new x or y coordinate, depending on what we're dragging
    switch (dragging) {
        case tl: bounds.x1 = x; bounds.y1 = y; break;
        case tr: bounds.x2 = x; bounds.y1 = y; break;
        case bl: bounds.x1 = x; bounds.y2 = y; break;
        case br: bounds.x2 = x; bounds.y2 = y; break;
        case t: bounds.y1 = y; break;
        case l: bounds.x1 = x; break;
        case b: bounds.y2 = y; break;
        case r: bounds.x2 = x; break;
    }
    
    // check if the right edge is left of the left one, and if so switch them
    if (bounds.x2 < bounds.x1) {
        // This macro was causing problems!!
        // swap(bounds.x1, bounds.x2);
        int temp = bounds.x1;
        bounds.x1 = bounds.x2;
        bounds.x2 = temp;

        if (dragging == tl) dragging = tr;
        else if (dragging == tr) dragging = tl;
        else if (dragging == bl) dragging = br;
        else if (dragging == br) dragging = bl;
        else if (dragging == l) dragging = r;
        else if (dragging == r) dragging = l;
    }
    
    // check if the bottom edge is above the top one, and if so switch them
    if (bounds.y2 < bounds.y1) {
        // This macro was causing problems!!
        // swap(bounds.y1, bounds.y2);
        int temp = bounds.y1;
        bounds.y1 = bounds.y2;
        bounds.y2 = temp;

        if (dragging == tl) dragging = bl;
        else if (dragging == bl) dragging = tl;
        else if (dragging == tr) dragging = br;
        else if (dragging == br) dragging = tr;
        else if (dragging == t) dragging = b;
        else if (dragging == b) dragging = t;
    }
    
    // make sure we stay inside the main image bounds
    if (bounds.x1 < 1) bounds.x1 = 1;
    if (bounds.y1 < 1) bounds.y1 = 1;
    if (bounds.x2 >= maxx) bounds.x2 = maxx-1;
    if (bounds.y2 >= maxy) bounds.y2 = maxy-1;
}

/*
 *  === place::enddrag ===
 *
 *  should be called when the mouse is released; stops dragging and ensures
 *  a size of at least minsize x minsize
 */

void place::enddrag(int maxx, int maxy) {
    const int minsize = 7;
    if (bounds.x2-bounds.x1 < minsize) {
        if (bounds.x1+minsize < maxx)
            bounds.x2 = bounds.x1+minsize;
        else
            bounds.x1 = bounds.x2-minsize;
    }
    if (bounds.y2-bounds.y1 < minsize) {
        if (bounds.y1+minsize < maxy)
            bounds.y2 = bounds.y1+minsize;
        else
            bounds.y1 = bounds.y2-minsize;
    }
    dragging = none;
}

/*
 *  === place::scanbounds ===
 *
 *  sets the bounds to absolute coordinates read from the next line
 *  of file f
 */

bool place::scanbounds(FILE *f) {
    int ret = fscanf(f, "%d,%d,%d,%d\n",
        &bounds.x1, &bounds.x2, &bounds.y1, &bounds.y2);
    return ret == EOF || ret == 0;
}

/*
 *  === place::printbounds ===
 *
 *  outputs the bounds to file f to be read later by scanbounds
 */

void place::printbounds(FILE *f) {
    fprintf(f, "%d,%d,%d,%d\n", bounds.x1, bounds.x2,
        bounds.y1, bounds.y2);
}

/*
 *  === place::saveto ===
 *
 *  saves the first (physically higher) digit to the ppm file "name"
 */

void place::saveto(const char *name) {
    digits[0].saveto(name);
}

/*
 *  === place::place ===
 *
 *  this is the constructor, which initializes variables
 */

place::place() {
    value = -1;
}

/*
 *  === place::~place ===
 *
 *  again, a destructor
 */

place::~place() {
    free(isgap);
    if (GDK_IS_PIXBUF(pixbuf)) gdk_pixbuf_unref(pixbuf);
}

