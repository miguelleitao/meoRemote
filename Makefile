LDLIBS=$(shell sdl-config --libs --cflags) -lSDL_gfx

all: meo_remote meo_remote.bmp

meo_remote.bmp: meo_remote_large.png
	convert $< -resize 170x640 $@

clean:
	rm -rf example

commit:
	git add .
	git commit -m "new update"

push: commit
	git push

pull:
	git pull
	git submodule update --recursive --remote


test:
	echo 'dynamically_created_rule1' >> .gitignore
	echo 'dynamically_created_rule2' >> .gitignore
