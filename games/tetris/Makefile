#	$NetBSD: Makefile,v 1.3.4.1 1997/11/20 09:06:53 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	tetris
SRCS=	input.c screen.c shapes.c scores.c tetris.c
MAN=	tetris.6
DPADD=	${LIBTERM}
LDADD=	-ltermcap
HIDEGAME=hidegame
SETGIDGAME=yes

.include <bsd.prog.mk>
