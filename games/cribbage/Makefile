#	$NetBSD: Makefile,v 1.13 2010/02/06 23:45:25 he Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PROG=	cribbage
DPADD=	${LIBCURSES} ${LIBTERMINFO}
LDADD=	-lcurses -lterminfo
SRCS=	extern.c crib.c cards.c instr.c io.c score.c support.c
MAN=	cribbage.6
HIDEGAME=hidegame
SETGIDGAME=yes
.if ${MKSHARE} != "no"
FILES=	cribbage.n
FILESNAME=cribbage.instr
FILESDIR=/usr/share/games
.endif

.include <bsd.prog.mk>
