#	from: @(#)Makefile	5.17 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.7 1993/12/06 05:28:34 cgd Exp $

PROG=	gprof
SRCS=	gprof.c arcs.c dfn.c lookup.c ${MACHINE_ARCH}.c hertz.c \
	printgprof.c printlist.c
CFLAGS+=-I${.CURDIR}/../../lib/csu/${MACHINE_ARCH}

beforeinstall:
	@install -d -o root -g wheel -m 755 ${DESTDIR}/usr/share/misc
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${.CURDIR}/gprof.flat ${.CURDIR}/gprof.callg \
	    ${DESTDIR}/usr/share/misc

.include <bsd.prog.mk>
