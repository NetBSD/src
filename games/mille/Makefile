#	from: @(#)Makefile	8.1 (Berkeley) 5/31/93
#	$Id: Makefile,v 1.4 1994/12/22 09:34:50 cgd Exp $

PROG=	mille
SRCS=	comp.c end.c extern.c init.c mille.c misc.c move.c print.c \
	roll.c save.c types.c varpush.c
DPADD=	${LIBCURSES} ${LIBTERM} ${LIBCOMPAT}
LDADD=	-lcurses -ltermlib -lcompat
MAN=	mille.6
HIDEGAME=hidegame

.include <bsd.prog.mk>

