#	$NetBSD: Makefile,v 1.7 1997/05/29 00:40:03 mellon Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	mille
SRCS=	comp.c end.c extern.c init.c mille.c misc.c move.c print.c \
	roll.c save.c types.c varpush.c
DPADD=	${LIBCURSES} ${LIBTERMCAP} ${LIBCOMPAT}
LDADD=	-lcurses -ltermcap -lcompat
MAN=	mille.6
HIDEGAME=hidegame

.include <bsd.prog.mk>

