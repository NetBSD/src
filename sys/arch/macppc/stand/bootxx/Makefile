#	$NetBSD: Makefile,v 1.4.8.1 2002/01/10 19:45:53 thorpej Exp $

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
