#	from: @(#)Makefile	8.1 (Berkeley) 6/29/93
#	$Id: Makefile,v 1.10 1994/05/17 03:35:48 cgd Exp $

PROG=	gprof
SRCS=	gprof.c arcs.c dfn.c lookup.c ${MACHINE_ARCH}.c hertz.c \
	printgprof.c printlist.c

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${.CURDIR}/gprof.flat ${.CURDIR}/gprof.callg \
	    ${DESTDIR}/usr/share/misc

.include <bsd.prog.mk>
