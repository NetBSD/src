#	$NetBSD: Makefile,v 1.9 1999/09/14 18:07:21 jsm Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	snake
SRCS=	snake.c
MAN=	snake.6
DPADD=	${LIBM} ${LIBCURSES}
LDADD=	-lm -lcurses
HIDEGAME=hidegame
SETGIDGAME=yes

.include "../../Makefile.inc"
.include <bsd.prog.mk>
