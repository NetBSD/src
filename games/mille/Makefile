#	$NetBSD: Makefile,v 1.6 1997/05/23 23:13:35 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	mille
SRCS=	comp.c end.c extern.c init.c mille.c misc.c move.c print.c \
	roll.c save.c types.c varpush.c
DPADD=	${LIBCURSES} ${LIBCOMPAT}
LDADD=	-lcurses -lcompat
MAN=	mille.6
HIDEGAME=hidegame

.include <bsd.prog.mk>

