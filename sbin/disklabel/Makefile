#	from: @(#)Makefile	8.2 (Berkeley) 3/17/94
#	$Id: Makefile,v 1.10 1994/09/22 22:03:52 mycroft Exp $

PROG=	disklabel
SRCS=	disklabel.c dkcksum.c
MAN8=	disklabel.0
CLEANFILES=disklabel.5.0

.if !defined(NOMAN)
all: disklabel.5.0
.endif

.if ${MACHINE} == "amiga"
CFLAGS+= -D${MACHINE}
.endif

.if ${MACHINE} == "i386"
CFLAGS+= -DNUMBOOT=2
.endif

.if ${MACHINE} == "hp300"
CFLAGS+= -DNUMBOOT=1
.endif

.if ${MACHINE} == "i386"
CFLAGS+= -DRAWPARTITION=\'d\'
.endif

beforeinstall:
.if !defined(NOMAN)
	install ${COPY} -o ${MANOWN} -g ${MANGRP} -m ${MANMODE} disklabel.5.0 \
	    ${DESTDIR}${MANDIR}5/disklabel.0
.endif

.include <bsd.prog.mk>
