include config.mk

gmip: *.c termbox.h
	${CC} -o gmip gmip.c

clean:
	rm -f gmip gmip-*.tar.gz

dist: clean
	tar -cf gmip-${VERSION}.tar LICENSE Makefile readme.md demo.gmi \
        config.mk termbox.h *.c gmip.1
	gzip gmip-${VERSION}.tar

install: gmip
	mkdir -p ${PREFIX}/bin
	cp -f gmip ${PREFIX}/bin
	chmod 755 ${PREFIX}/bin/gmip
	mkdir -p ${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < gmip.1 > ${MANPREFIX}/man1/gmip.1
	chmod 644 ${MANPREFIX}/man1/gmip.1

uninstall:
	rm -f ${PREFIX}/bin/gmip ${MANPREFIX}/man1/edit.1

.PHONY: gmip clean dist install uninstall
