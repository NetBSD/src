#	from: @(#)Makefile	5.8 (Berkeley) 3/10/91
#	$Id: Makefile,v 1.4 1993/07/31 15:25:54 mycroft Exp $

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
