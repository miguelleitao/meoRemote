

EXEC=meoRemote

CFLAGS=-Wall -Wextra -O2
LDLIBS=$(shell sdl-config --libs --cflags) -lSDL_gfx

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

all: ${EXEC} meo_remote.bmp button_list.txt

meo_remote.bmp: meo_remote_large.png
	convert $< -resize 170x640 $@

button_list.txt: ${EXEC}
	./$< -l | cut -c4- |sort -n >$@

clean:
	rm -rf ${EXEC} meo_remote.bmp button_list.txt

commit:
	git add .
	git commit -m "new update"

push: commit
	git push

pull:
	git pull
	git submodule update --recursive --remote

install: ${EXEC}
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 755 ${EXEC} $(DESTDIR)$(PREFIX)/bin/

test:
	echo 'dynamically_created_rule1' >> .gitignore
	echo 'dynamically_created_rule2' >> .gitignore
