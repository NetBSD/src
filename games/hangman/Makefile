#	$NetBSD: Makefile,v 1.7 1998/02/18 22:37:31 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	hangman
SRCS=	endgame.c extern.c getguess.c getword.c main.c playgame.c \
	prdata.c prman.c prword.c setup.c
MAN=	hangman.6
DPADD=	${LIBCURSES}
LDADD=	-lcurses
HIDEGAME=hidegame

.include <bsd.prog.mk>


