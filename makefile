PKGS = gtkmm-3.0 gstreamermm-1.0

CC = g++
FLAGS = -Wall -O3 -std=c++11
CFLAGS = $(FLAGS) `pkg-config --cflags $(PKGS)`
LDFLAGS = $(FLAGS) `pkg-config --libs $(PKGS)`

SRCS = main.cpp app.cpp app.h pipeline.cpp pipeline.h config.h README makefile
OBJS = main.o pipeline.o app.o

.cpp.o:
	$(CC) -c $(CFLAGS) $<

all: watermeter

watermeter: $(OBJS)
	$(CC) -o watermeter $(OBJS) $(LDFLAGS)

run: watermeter
	./watermeter

clean:
	rm -f $(OBJS) watermeter.zip

zip: $(SRCS)
	zip watermeter.zip $(SRCS) makefile

main.o: main.cpp app.h pipeline.h makefile
app.o: app.cpp app.h config.h makefile
pipeline.o: pipeline.cpp pipeline.h makefile

