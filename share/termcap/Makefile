#	@(#)Makefile	5.12 (Berkeley) 7/1/90

# reorder gives an editor command for most common terminals
# (in reverse order from n'th to 1'st most commonly used)
# to move them to the front of termcap
#
MAN5=	termcap.0

all: termcap

termcap: reorder termcap.src
#	ex - ${.CURDIR}/termcap.src < ${.CURDIR}/reorder > /dev/null
	-ex - ${.CURDIR}/termcap.src < ${.CURDIR}/reorder > /dev/null

clean cleandir:
	rm -f termcap

beforeinstall:
	@if [ ! -d ${DESTDIR}${BINDIR}/misc ]; then \
                /bin/rm -f ${DESTDIR}${BINDIR}/misc ; \
                mkdir -p ${DESTDIR}${BINDIR}/misc ; \
                chown root.wheel ${DESTDIR}${BINDIR}/misc ; \
                chmod 755 ${DESTDIR}${BINDIR}/misc ; \
        else \
                true ; \
        fi
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 termcap \
	     ${DESTDIR}${BINDIR}/misc
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/map3270 \
	     ${DESTDIR}${BINDIR}/misc

.include <bsd.prog.mk>
