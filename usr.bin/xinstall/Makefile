#	$NetBSD: Makefile,v 1.6 1994/11/14 04:57:16 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	xinstall
SRCS=	stat_flags.c xinstall.c
MAN1=	install.0
.PATH:	${.CURDIR}/../../bin/ls

realinstall:
	install ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${PROG} ${DESTDIR}${BINDIR}/install

.include <bsd.prog.mk>
