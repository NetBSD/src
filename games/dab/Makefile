#	Makefile,v 1.5 2006/05/11 23:16:28 mrg Exp

DPADD+=${LIBCURSES} ${LIBTERMCAP} ${LIBM}
LDADD+=-lcurses -ltermcap -lm

PROG_CXX=dab
MAN=dab.6
SRCS=algor.cc board.cc main.cc human.cc box.cc player.cc gamescreen.cc \
    ttyscrn.cc random.cc

.include <bsd.prog.mk>
