#	from: @(#)Makefile	5.11 (Berkeley) 2/28/91
#	$Id: Makefile,v 1.3 1993/08/01 05:44:15 mycroft Exp $

PROG=	snake
SRCS=	snake.c move.c
MAN6=	snake.0
DPADD=	${LIBM} ${LIBTERM}
LDADD=	-lm -ltermcap
HIDEGAME=hidegame

.include "../../Makefile.inc"
.include <bsd.prog.mk>
