#	$NetBSD: Makefile,v 1.2 1999/02/13 02:54:46 lukem Exp $

PROG= bootxx
MKMAN=	no
STRIPFLAG=
BINMODE=444

CFLAGS=-O2 -Wall

${PROG}: ${PROG}.o
	${LD} -o bootxx -x -N -Ttext 4000 -e _start ${PROG}.o
	@size ${PROG}

.include <bsd.prog.mk>
