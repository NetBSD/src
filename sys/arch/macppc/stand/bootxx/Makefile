#	$NetBSD: Makefile,v 1.3.2.1 2000/11/20 20:13:03 bouyer Exp $

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
