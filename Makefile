LDLIBS=$(shell sdl-config --libs --cflags) -lSDL_gfx

all: meo_remote

clean:
	rm -rf example

