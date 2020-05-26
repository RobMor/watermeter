PKGS = gtk+-3.0 cairo gstreamer-1.0 gstreamer-app-1.0

CC = g++
FLAGS = -g -Wall
CFLAGS = $(FLAGS) `pkg-config --cflags $(PKGS)`
LDFLAGS = $(FLAGS) `pkg-config --libs $(PKGS)`

SRCS = main.cpp app.cpp app.h pipeline.cpp pipeline.h config.h makefile
OBJS = main.o pipeline.o app.o

.cpp.o:
	$(CC) -c $(CFLAGS) $<

all: ocr

ocr: $(OBJS)
	$(CC) -o ocr $(OBJS) $(LDFLAGS)

run: ocr
	./ocr

clean:
	rm -f $(OBJS) watermeter.zip

zip: $(SRCS)
	zip watermeter.zip $(SRCS) makefile

main.o: main.cpp app.h pipeline.h makefile
app.o: app.cpp app.h config.h makefile
pipeline.o: pipeline.cpp pipeline.h makefile

