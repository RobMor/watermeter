PKGS = gtk+-3.0 cairo gstreamer-1.0 gstreamer-app-1.0

CC = g++
FLAGS = -g
CFLAGS = $(FLAGS) `pkg-config --cflags $(PKGS)`
LDFLAGS = $(FLAGS) `pkg-config --libs $(PKGS)`

OBJS = main.o pipeline.o app.o

.cpp.o:
	$(CC) -c $(CFLAGS) $<

all: ocr

ocr: $(OBJS)
	$(CC) -o ocr $(OBJS) $(LDFLAGS)

run: ocr
	./ocr

clean:
	rm -f $(OBJS)

main.o: main.cpp app.h pipeline.h
app.o: app.cpp app.h
pipeline.o: pipeline.cpp pipeline.h

