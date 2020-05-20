#include "ppm.h"

// used to read through each line of the ppm, ignoring comments (assumes variable "char *i" exists)
#define nextline() do {for (; *i != '\n' || *(i+1) == '#'; i++); i++;} while (0)

/*
 *	PPM file format
 *
 *	Any line starting with a '#' is a comment. The first line contains "P6"
 *	if the data is in binary rgb format, which is the only format this
 *	reads or writes. The second line contains the width and height,
 *	separated by a space, as base ten numbers. The third line contains the
 *	maximum pixel value, which I assume will always be 255 for binary data.
 *	The fourth line is the start of the data itself.
 *
 */

/*
 *	=== loadfile ===
 *
 *	opens ppm file "name" and reads width, height, and image data into img.
 *	img->data should later be freed with free(img->data); unless it is
 *	passed again to this function
 *
 *	returns 0 on success, nonzero on failure
 */

int loadfile(const char *name, image *img) {
	
	// open the file, and check for errors
	int file = open(name, O_RDONLY);
	if (file == -1) {
		perror(name);
		return 1;
	}
	
	// read in the first chunk of the file
	size_t size;
	char buf[512];
	size = read(file, buf, 512);
	
	// check for proper "P6" header
	if (buf[0] != 'P' || buf[1] != '6') {
		puts("Only supported filetype is binary PPM.");
		close(file);
		return 1;
	}
	
	// read in, line by line, width, heigh, and maximum pixel value
	char *i = buf+2;
	nextline();
	img->width = atoi(i);
	for (; *i != ' '; i++); i++;
	img->height = atoi(i);
	nextline();
	//maxval = atoi(i); ignored...
	nextline();
	
	// reallocate image according to new width and height, and make sure it succeeds
	free(img->data);
	img->data = (byte*) malloc(img->width*img->height*3);
	if (!img->data) {
		perror("Allocation failed");
		close(file);
		return 1;
	}
	
	// starting after header data, start reading in to the image
	size += buf-i;
	memcpy(img->data, i, size);
	
	// read the rest until size is zero, indicating EOF
	for (int dataoffs = size; size; dataoffs += size) {
		size = read(file, buf, 512);
		memcpy(img->data+dataoffs, buf, size);
	}
	
	// close file
	close(file);
	
	// success!
	return 0;
}

/*
 *	=== savefile ===
 *
 *	saves a ppm file called name from the data in img
 */

void savefile(const char *name, image *img) {
	
	// open file
	FILE *file = fopen(name, "w");
	
	// write header, with "P6", width, height, and max pixel value
	fprintf(file, "P6\n%d %d\n255\n", img->width, img->height);
	
	// write image data
	fwrite(img->data, 1, img->width*img->height*3, file);
	
	// close file
	fclose(file);
	
}

