#	from: @(#)Makefile	5.11 (Berkeley) 2/28/91
#	$Id: Makefile,v 1.4 1994/05/23 05:37:40 cgd Exp $

PROG=	snake
SRCS=	snake.c move.c
MAN6=	snake.0
DPADD=	${LIBM} ${LIBTERM} ${LIBCOMPAT}
LDADD=	-lm -ltermcap -lcompat
HIDEGAME=hidegame

.include "../../Makefile.inc"
.include <bsd.prog.mk>
