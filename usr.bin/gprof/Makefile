#	$NetBSD: Makefile,v 1.15 1996/10/18 06:14:49 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/29/93

PROG=	gprof
SRCS=	gprof.c arcs.c dfn.c lookup.c ${MACHINE_ARCH}.c hertz.c \
	printgprof.c printlist.c
CFLAGS+= -D${MACHINE_ARCH}

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${.CURDIR}/gprof.flat ${.CURDIR}/gprof.callg \
	    ${DESTDIR}/usr/share/misc

.if make(install)
SUBDIR+= PSD.doc
.endif

.include <bsd.prog.mk>
