#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.5 1994/09/19 03:34:06 mycroft Exp $

PROG=	ar
CFLAGS+=-I${.CURDIR}
SRCS=	append.c ar.c archive.c contents.c delete.c extract.c misc.c \
	move.c print.c replace.c 
MAN1=	ar.0
CLEANFILES=ar.5.0

ar.0: ar.5.0

.if !defined(NOMAN)
afterinstall:
	install ${COPY} -o ${MANOWN} -g ${MANGRP} -m ${MANMODE} ar.5.0 \
	    ${DESTDIR}${MANDIR}5/ar.0
.endif

.include <bsd.prog.mk>
