#	$NetBSD: Makefile,v 1.11 1998/09/29 07:33:21 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	cribbage
DPADD=	${LIBCURSES}
LDADD=	-lcurses
SRCS=	extern.c crib.c cards.c instr.c io.c score.c support.c
MAN=	cribbage.6
HIDEGAME=hidegame
SETGIDGAME=yes
.if !defined(NOSHARE)
FILES=	cribbage.n
FILESNAME=cribbage.instr
FILESDIR=/usr/share/games
.endif

.include <bsd.prog.mk>
