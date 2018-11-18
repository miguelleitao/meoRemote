

EXEC=meoRemote
DESKTOP=${EXEC}.desktop
ICON=${EXEC}-icon.png
SKIN=${EXEC}-skin.bmp
CONFIG=${EXEC}.conf
MAIN_SKIN=$(DESTDIR)$(PREFIX)/share/${EXEC}/images/${SKIN}


TARGETS=${EXEC} ${DESKTOP} ${ICON} ${SKIN} ${CONFIG} button_list.txt

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

DEFS=-D CONFIG=\"${CONFIG}\" -D SKIN=\"${SKIN}\"
CFLAGS=-Wall -Wextra -O2 ${DEFS}
LDLIBS=$(shell sdl-config --libs --cflags) -lSDL_gfx

normal: ${EXEC} ${SKIN} ${CONFIG}

all: ${TARGETS} 

${SKIN}: meo_remote_large.png
	convert $< -resize 170x640 $@

${ICON}: meo_remote_icon_large.png
	convert $< -resize 128x128 $@

config:
	rm -f ${CONFIG}
	@make ${CONFIG}

${CONFIG}:
	./findBox -c >$@
	@main_skin=$(DESTDIR)$(PREFIX)/share/${EXEC}/images/${SKIN}; \
	if [ ! -f $$main_skin ]; \
	then \
	    main_skin=${SKIN}; \
	fi; \
	echo "skin $$main_skin" >>$@

button_list.txt: ${EXEC}
	./$< -l | cut -c4- |sort -n >$@

clean:
	-rm -rf ${TARGETS} 

commit:
	git add .
	git commit -m "new update"

push: commit
	git push

pull:
	git pull
	git submodule update --recursive --remote

${DESKTOP}:
	@echo "[Desktop Entry]" 					 >$@
	@echo "Name=${EXEC}"					>>$@
	@echo "Comment=Remote controller for meoBox"		>>$@
	@echo "Comment[pt]=Controlador remoto da meoBox"		>>$@
	@echo "Exec=$(DESTDIR)$(PREFIX)/bin/${EXEC}"		>>$@
	@echo "Icon=$(DESTDIR)$(PREFIX)/share/pixmaps/${ICON}"	>>$@
	@echo "Type=Application"					>>$@
	@echo "Categories=Utility,Multimedia"			>>$@
	@echo "Terminal=false"					>>$@


install: all
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

