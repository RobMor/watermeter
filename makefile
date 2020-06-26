PKGS = gtk+-3.0 cairo gstreamer-1.0 gstreamer-app-1.0

CXX = g++
FLAGS = -Wall
CPPFLAGS = $(FLAGS) $(shell pkg-config --cflags $(PKGS))
LDLIBS = $(shell pkg-config --libs $(PKGS))

EXEC = watermeter
SRCDIR = src
BUILDDIR = target

SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SRCS))

.PHONY: all directories run format clean cleanall

all: directories $(EXEC)

directories: $(BUILDDIR)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(EXEC): $(OBJS)
	$(CXX) $(LDLIBS) -o $(EXEC) $(OBJS)

$(OBJS): $(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CPPFLAGS) -o $@ -c $<

run: all
	./$(EXEC)

format:
	clang-format -i $(wildcard $(SRCDIR)/*)

clean:
	rm -rf $(BUILDDIR)

cleanall: clean
	rm $(EXEC)
