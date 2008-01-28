#	$NetBSD: Makefile,v 1.6 2008/01/28 07:03:59 dholland Exp $

DPADD+=${LIBCURSES} ${LIBTERMCAP} ${LIBM}
LDADD+=-lcurses -ltermcap -lm

PROG_CXX=dab
MAN=dab.6
SRCS=algor.cc board.cc main.cc human.cc box.cc player.cc gamescreen.cc \
    ttyscrn.cc random.cc

.include <bsd.prog.mk>
