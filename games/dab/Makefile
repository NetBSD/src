#	$NetBSD: Makefile,v 1.1.1.1 2003/12/26 17:57:02 christos Exp $

WARNS=2
DPADD+=${LIBSTDCPP} ${LIBCURSES} ${LIBTERMCAP} ${LIBM}
LDADD+=-lstdc++ -lcurses -ltermcap -lm

PROG=dab
MAN=dab.6
SRCS=algor.C board.C main.C human.C box.C player.C gamescreen.C \
    ttyscrn.C random.C

.include <bsd.prog.mk>
