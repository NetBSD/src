#	$NetBSD: Makefile,v 1.3 1999/05/06 01:04:48 wrstuden Exp $

PROG= bootxx
MKMAN=	no
STRIPFLAG=
BINMODE=444

CFLAGS=-O2 -Wall

${PROG}: ${PROG}.o
	${LD} -o bootxx -x -N -Ttext 4000 -e _start ${PROG}.o
	@${SIZE} ${PROG}

.include <bsd.prog.mk>
