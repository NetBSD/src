#	$NetBSD: Makefile,v 1.8 2010/02/06 23:45:25 he Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	hangman
SRCS=	endgame.c extern.c getguess.c getword.c main.c playgame.c \
	prdata.c prman.c prword.c setup.c
MAN=	hangman.6
DPADD=	${LIBCURSES} ${LIBTERMINFO}
LDADD=	-lcurses -lterminfo
HIDEGAME=hidegame

.include <bsd.prog.mk>


