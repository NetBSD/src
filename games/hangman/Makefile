#	from: @(#)Makefile	5.7 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.3 1993/08/01 05:45:09 mycroft Exp $

PROG=	hangman
SRCS=	endgame.c extern.c getguess.c getword.c main.c playgame.c \
	prdata.c prman.c prword.c setup.c
MAN6=	hangman.0
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
HIDEGAME=hidegame

.include <bsd.prog.mk>


