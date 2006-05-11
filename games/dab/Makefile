#	$NetBSD: Makefile,v 1.5 2006/05/11 23:16:28 mrg Exp $

WARNS=3
DPADD+=${LIBCURSES} ${LIBTERMCAP} ${LIBM}
LDADD+=-lcurses -ltermcap -lm

PROG_CXX=dab
MAN=dab.6
SRCS=algor.cc board.cc main.cc human.cc box.cc player.cc gamescreen.cc \
    ttyscrn.cc random.cc

.include <bsd.own.mk>
 
.if ${HAVE_GCC} == 4
NOGCCERROR=1
.endif

.include <bsd.prog.mk>
