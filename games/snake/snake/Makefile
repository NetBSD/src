#	$NetBSD: Makefile,v 1.7 1997/11/20 00:12:41 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	snake
SRCS=	snake.c move.c
MAN=	snake.6
DPADD=	${LIBM} ${LIBTERM} ${LIBCOMPAT}
LDADD=	-lm -ltermcap -lcompat
HIDEGAME=hidegame
SETGIDGAME=yes

.include "../../Makefile.inc"
.include <bsd.prog.mk>
