#	from: @(#)Makefile	5.11 (Berkeley) 2/28/91
#	$Id: Makefile,v 1.5 1994/12/22 09:36:14 cgd Exp $

PROG=	snake
SRCS=	snake.c move.c
MAN=	snake.6
DPADD=	${LIBM} ${LIBTERM} ${LIBCOMPAT}
LDADD=	-lm -ltermcap -lcompat
HIDEGAME=hidegame

.include "../../Makefile.inc"
.include <bsd.prog.mk>
