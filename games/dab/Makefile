#	$NetBSD: Makefile,v 1.3 2004/01/05 15:35:59 christos Exp $

WARNS=2
DPADD+=${LIBCURSES} ${LIBTERMCAP} ${LIBM}
LDADD+=-lcurses -ltermcap -lm

PROG_CXX=dab
MAN=dab.6
SRCS=algor.cc board.cc main.cc human.cc box.cc player.cc gamescreen.cc \
    ttyscrn.cc random.cc

.include <bsd.prog.mk>
