#	$NetBSD: Makefile,v 1.9 1998/02/05 12:21:32 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	cribbage
DPADD=	${LIBCURSES} ${LIBTERMCAP}
LDADD=	-lcurses -ltermcap
SRCS=	extern.c crib.c cards.c instr.c io.c score.c support.c
MAN=	cribbage.6
HIDEGAME=hidegame
SETGIDGAME=yes
FILES=	cribbage.n
FILESNAME=cribbage.instr
FILESDIR=/usr/share/games

.include <bsd.prog.mk>
