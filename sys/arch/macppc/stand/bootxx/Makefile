#	$NetBSD: Makefile,v 1.4 2000/02/07 16:27:00 tsubai Exp $

PROG= bootxx
MKMAN=	no
STRIPFLAG=
BINMODE=444

CFLAGS= -O2 -Wall -msoft-float
CPPFLAGS+= -D_STANDALONE

${PROG}: ${PROG}.o
	${LD} -o bootxx -x -N -Ttext 4000 -e _start ${PROG}.o
	@${SIZE} ${PROG}

.include <bsd.prog.mk>
