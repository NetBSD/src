#	$NetBSD: Makefile,v 1.8 1996/10/18 06:25:09 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	xinstall
SRCS=	stat_flags.c xinstall.c
MAN=	install.1
.PATH:	${.CURDIR}/../../bin/ls

realinstall:
	${INSTALL} ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${PROG} ${DESTDIR}${BINDIR}/install

.include <bsd.prog.mk>
