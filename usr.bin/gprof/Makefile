#	@(#)Makefile	5.17 (Berkeley) 5/11/90

PROG=	gprof
SRCS=	gprof.c arcs.c dfn.c lookup.c ${MACHINE}.c hertz.c \
	printgprof.c printlist.c
CFLAGS+=-I${.CURDIR}/../../lib/csu/${MACHINE}

beforeinstall:
	@if [ ! -d ${DESTDIR}/usr/share/misc ]; then \
                /bin/rm -f ${DESTDIR}/usr/share/misc ; \
                mkdir -p ${DESTDIR}/usr/share/misc ; \
                chown root.wheel ${DESTDIR}/usr/share/misc ; \
                chmod 755 ${DESTDIR}/usr/share/misc ; \
        else \
                true ; \
        fi
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/gprof.flat ${.CURDIR}/gprof.callg \
	    ${DESTDIR}/usr/share/misc

.include <bsd.prog.mk>
