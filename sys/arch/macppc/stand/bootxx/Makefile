#	$NetBSD: Makefile,v 1.2.4.1 1999/06/21 00:51:42 thorpej Exp $

PROG= bootxx
MKMAN=	no
STRIPFLAG=
BINMODE=444

CFLAGS=-O2 -Wall

${PROG}: ${PROG}.o
	${LD} -o bootxx -x -N -Ttext 4000 -e _start ${PROG}.o
	@${SIZE} ${PROG}

.include <bsd.prog.mk>
