#	$Id: Makefile,v 1.1.1.1 1995/07/03 20:56:39 cgd Exp $

.PATH:	${.CURDIR}/../lint1

PROG=	xlint
SRCS=	xlint.c mem.c
MAN=	lint.1

CFLAGS+=-I${.CURDIR}/../lint1

realinstall:
	install ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${PROG} ${DESTDIR}${BINDIR}/lint


.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.prog.mk>
