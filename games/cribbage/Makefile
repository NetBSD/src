#	$NetBSD: Makefile,v 1.10 1998/02/18 22:37:31 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	cribbage
DPADD=	${LIBCURSES}
LDADD=	-lcurses
SRCS=	extern.c crib.c cards.c instr.c io.c score.c support.c
MAN=	cribbage.6
HIDEGAME=hidegame
SETGIDGAME=yes
FILES=	cribbage.n
FILESNAME=cribbage.instr
FILESDIR=/usr/share/games

.include <bsd.prog.mk>
