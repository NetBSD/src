#	$NetBSD: Makefile,v 1.6 2002/04/13 13:44:48 tsutsui Exp $

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
