#	$NetBSD: Makefile,v 1.3 1996/10/18 06:26:30 thorpej Exp $

.PATH:	${.CURDIR}/../lint1

PROG=	xlint
SRCS=	xlint.c mem.c
MAN=	lint.1

CFLAGS+=-I${.CURDIR}/../lint1

realinstall:
	${INSTALL} ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${PROG} ${DESTDIR}${BINDIR}/lint


.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.prog.mk>
