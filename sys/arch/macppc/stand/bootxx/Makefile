#	$NetBSD: Makefile,v 1.5 2001/12/12 01:49:45 tv Exp $

PROG= bootxx
NOMAN=	# defined
STRIPFLAG=
BINMODE=444

CFLAGS= -O2 -Wall -msoft-float
CPPFLAGS+= -D_STANDALONE

${PROG}: ${PROG}.o
	${LD} -o bootxx -x -N -Ttext 4000 -e _start ${PROG}.o
	@${SIZE} ${PROG}

.include <bsd.prog.mk>
