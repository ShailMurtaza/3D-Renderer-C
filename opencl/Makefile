CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lm
SDL_CFLAGS = $(shell pkg-config --cflags sdl3 2>/dev/null || echo "-I/usr/include/SDL3")
SDL_LIBS = $(shell pkg-config --libs sdl3 2>/dev/null || echo "-lSDL3")
SDL_TTF_CFLAGS = $(shell pkg-config --cflags sdl3-ttf 2>/dev/null || echo "-I/usr/include/SDL3_ttf")
SDL_TTF_LIBS = $(shell pkg-config --libs sdl3-ttf 2>/dev/null || echo "-lSDL3_ttf")
OPENCL_CFLAGS = $(shell pkg-config --cflags OpenCL 2>/dev/null) -DCL_TARGET_OPENCL_VERSION=300
OPENCL_LIBS = $(shell pkg-config --libs OpenCL 2>/dev/null || echo "-lOpenCL")

all: renderer

renderer: main.o transformations.o clip.o obj_loader.o events.o opencl_utils.o
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(SDL_TTF_CFLAGS) $(OPENCL_CFLAGS) -o $@ $^ $(LDFLAGS) $(SDL_LIBS) $(SDL_TTF_LIBS) $(OPENCL_LIBS)

main.o: main.c transformations.h clip.h obj_loader.h events.h opencl_utils.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(SDL_TTF_CFLAGS) $(OPENCL_CFLAGS) -c -o $@ $<

transformations.o: transformations.c transformations.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(OPENCL_CFLAGS) -c -o $@ $<

clip.o: clip.c clip.h transformations.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(OPENCL_CFLAGS) -c -o $@ $<

obj_loader.o: obj_loader.c obj_loader.h transformations.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(OPENCL_CFLAGS) -c -o $@ $<

events.o: events.c events.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(OPENCL_CFLAGS) -c -o $@ $<

opencl_utils.o: opencl_utils.c opencl_utils.h
	$(CC) $(CFLAGS) $(OPENCL_CFLAGS) -c -o $@ $<

clean:
	rm -f *.o renderer

.PHONY: all clean
