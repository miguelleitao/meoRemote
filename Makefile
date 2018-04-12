LDLIBS=$(shell sdl-config --libs --cflags) -lSDL_gfx

all: meo_remote meo_remote.bmp

meo_remote.bmp: meo_remote_large.png
	convert $< -resize 170x640 $@

clean:
	rm -rf example

