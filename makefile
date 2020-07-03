PKGS = gtkmm-3.0 gstreamermm-1.0

CXX      = g++
CFLAGS = -Wall -Wextra -O3 -I. `pkg-config --cflags $(PKGS)`
LIBS = `pkg-config --libs $(PKGS)`

EXEC = watermeter
SRCDIR = src
BUILDDIR = target

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SOURCES))

all: directories $(EXEC)

$(EXEC): $(OBJECTS)
	$(CXX) $(CFLAGS) $(OBJECTS) $(LIBS) -o $(EXEC)

$(OBJECTS): $(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

directories: $(BUILDDIR)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run: all
	./$(EXEC)

format:
	clang-format -i $(wildcard $(SRCDIR)/*)

clean:
	rm -rf $(BUILDDIR)

cleanall: clean
	rm $(EXEC)

.PHONY: all directories run format clean cleanall
