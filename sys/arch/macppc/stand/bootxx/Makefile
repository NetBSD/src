#	$NetBSD: Makefile,v 1.4.12.1 2002/01/08 00:26:13 nathanw Exp $

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
