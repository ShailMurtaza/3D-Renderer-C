CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lm
SDL_CFLAGS = $(shell pkg-config --cflags sdl3 2>/dev/null || echo "-I/usr/include/SDL3")
SDL_LIBS = $(shell pkg-config --libs sdl3 2>/dev/null || echo "-lSDL3")
SDL_TTF_CFLAGS = $(shell pkg-config --cflags sdl3-ttf 2>/dev/null || echo "-I/usr/include/SDL3_ttf")
SDL_TTF_LIBS = $(shell pkg-config --libs sdl3-ttf 2>/dev/null || echo "-lSDL3_ttf")

all: renderer

renderer: main.o transformations.o clip.o obj_loader.o events.o
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(SDL_TTF_CFLAGS) -o $@ $^ $(LDFLAGS) $(SDL_LIBS) $(SDL_TTF_LIBS)

main.o: main.c transformations.h clip.h obj_loader.h events.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(SDL_TTF_CFLAGS) -c -o $@ $<

transformations.o: transformations.c transformations.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c -o $@ $<

clip.o: clip.c clip.h transformations.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c -o $@ $<

obj_loader.o: obj_loader.c obj_loader.h transformations.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c -o $@ $<

events.o: events.c events.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c -o $@ $<

clean:
	rm -f *.o renderer

.PHONY: all clean
