#	$NetBSD: Makefile,v 1.1 1998/06/12 21:07:24 tsubai Exp $

PROG= bootxx
NOMAN=
STRIPFLAG=
BINMODE=444

CFLAGS=-O2 -Wall

${PROG}: ${PROG}.o
	${LD} -o bootxx -x -N -Ttext 4000 -e _start ${PROG}.o
	@size ${PROG}

.include <bsd.prog.mk>
