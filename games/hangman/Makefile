#	from: @(#)Makefile	5.7 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.4 1994/12/22 09:34:37 cgd Exp $

PROG=	hangman
SRCS=	endgame.c extern.c getguess.c getword.c main.c playgame.c \
	prdata.c prman.c prword.c setup.c
MAN=	hangman.6
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
HIDEGAME=hidegame

.include <bsd.prog.mk>


