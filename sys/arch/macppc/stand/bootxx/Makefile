#	$NetBSD: Makefile,v 1.4.12.2 2002/04/17 00:03:43 nathanw Exp $

PROG= bootxx
NOMAN=	# defined
STRIPFLAG=
BINMODE=444

CFLAGS= -Os -mmultiple -Wall -msoft-float
CPPFLAGS+= -D_STANDALONE

${PROG}: ${PROG}.o
	${LD} -o bootxx -x -N -Ttext 4000 -e _start ${PROG}.o
	@${SIZE} ${PROG}

.include <bsd.prog.mk>
