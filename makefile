PKGS = gtk+-2.0 gstreamer-1.0 gstreamer-app-1.0 tesseract libtiff-4

CC = g++
FLAGS = -O2 -g
CFLAGS = $(FLAGS) `pkg-config --cflags $(PKGS)`
LDFLAGS = $(FLAGS) `pkg-config --libs $(PKGS)`

OBJS = main.o ppm.o digit.o input.o

.cpp.o:
	$(CC) -c $(CFLAGS) $<

all: ocr

ocr: $(OBJS)
	$(CC) -o ocr $(OBJS) $(LDFLAGS)

run: ocr
	./ocr

clean:
	rm -f $(OBJS)

main.o: main.cpp main.h ppm.h digit.h input.h
ppm.o: ppm.cpp ppm.h main.h
digit.o: digit.cpp digit.h ppm.h main.h
input.o: input.cpp input.h ppm.h main.h

