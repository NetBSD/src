#	$NetBSD: Makefile,v 1.2 2003/12/27 01:16:55 christos Exp $

WARNS=2
DPADD+=${LIBSTDCPP} ${LIBCURSES} ${LIBTERMCAP} ${LIBM}
LDADD+=-lstdc++ -lcurses -ltermcap -lm

PROG=dab
MAN=dab.6
SRCS=algor.cc board.cc main.cc human.cc box.cc player.cc gamescreen.cc \
    ttyscrn.cc random.cc

.include <bsd.prog.mk>
