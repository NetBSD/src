#	$NetBSD: Makefile,v 1.8 1998/02/04 10:24:13 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	snake
SRCS=	snake.c move.c
MAN=	snake.6
DPADD=	${LIBM} ${LIBTERMCAP}
LDADD=	-lm -ltermcap
HIDEGAME=hidegame
SETGIDGAME=yes

.include "../../Makefile.inc"
.include <bsd.prog.mk>
