#	$NetBSD: Makefile,v 1.7 1997/03/24 22:15:41 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	cribbage
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
SRCS=	extern.c crib.c cards.c instr.c io.c score.c support.c
MAN=	cribbage.6
HIDEGAME=hidegame
FILES=	cribbage.n
FILESNAME=cribbage.instr
FILESDIR=/usr/share/games

.include <bsd.prog.mk>
