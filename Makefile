include config.mk

gmip: *.c termbox.h
	${CC} -o gmip gmip.c

clean:
	rm -f gmip gmip-*.tar.gz

dist: clean
	tar -cf gmip-${VERSION}.tar LICENSE Makefile readme.md *.md \
		config.mk termbox.h *.c
	gzip gmip-${VERSION}.tar

install: gmip
	mkdir -p ${PREFIX}/bin
	cp -f edit ${PREFIX}/bin
	chmod 755 ${PREFIX}/bin/gmip

uninstall:
	rm -f ${PREFIX}/bin/gmip

.PHONY: gmip clean dist install uninstall
