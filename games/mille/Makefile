#	from: @(#)Makefile	5.10 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.2 1993/08/01 05:45:03 mycroft Exp $

PROG=	mille
SRCS=	comp.c end.c extern.c init.c mille.c misc.c move.c print.c \
	roll.c save.c types.c varpush.c
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
MAN6=	mille.0
HIDEGAME=hidegame

.include <bsd.prog.mk>

