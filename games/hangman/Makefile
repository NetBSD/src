#	$NetBSD: Makefile,v 1.6 1998/02/05 12:23:04 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	hangman
SRCS=	endgame.c extern.c getguess.c getword.c main.c playgame.c \
	prdata.c prman.c prword.c setup.c
MAN=	hangman.6
DPADD=	${LIBCURSES} ${LIBTERMCAP}
LDADD=	-lcurses -ltermcap
HIDEGAME=hidegame

.include <bsd.prog.mk>


