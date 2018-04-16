

EXEC=meoRemote
DESKTOP=${EXEC}.desktop
ICON=${EXEC}-icon.png
SKIN=${EXEC}-skin.bmp
CONFIG=${EXEC}.conf

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

DEFS=-D CONFIG=\"${CONFIG}\" -D SKIN=\"${SKIN}\"
CFLAGS=-Wall -Wextra -O2 ${DEFS}
LDLIBS=$(shell sdl-config --libs --cflags) -lSDL_gfx

all: ${EXEC} meo_remote.bmp button_list.txt

${SKIN}: meo_remote_large.png
	convert $< -resize 170x640 $@

${ICON}: meo_remote_icon_large.png
	convert $< -resize 64x64 $@

config:
	rm ${CONFIG}
	@make ${CONFIG}

${CONFIG}:
	./findBox -c >$@
	echo "skin=$(DESTDIR)$(PREFIX)/share/${EXEC}/images/${SKIN}" >>$@

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
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 644 $(ICON) $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -d $(DESTDIR)$(PREFIX)/share/applications/
	install -m 644 ${DESKTOP} $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)$(PREFIX)/share/${EXEC}/images/
	install -m 644 ${SKIN} $(DESTDIR)$(PREFIX)/share/${EXEC}/images/
	install -d $(DESTDIR)$(PREFIX)/share/${EXEC}/config/
	install -m 644 ${CONFIG} $(DESTDIR)$(PREFIX)/share/${EXEC}/config/

