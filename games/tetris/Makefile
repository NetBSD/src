#	$NetBSD: Makefile,v 1.5 2008/01/28 01:38:59 dholland Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	tetris
SRCS=	input.c screen.c shapes.c scores.c tetris.c
MAN=	tetris.6
DPADD=	${LIBTERM}
LDADD=	-ltermcap
HIDEGAME=hidegame
SETGIDGAME=yes
WARNS=	4

.include <bsd.prog.mk>
