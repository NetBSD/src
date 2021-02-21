# $NetBSD: Makefile,v 1.2 2021/02/21 16:05:44 rillig Exp $

PROG=		cgram
DPADD=		${LIBCURSES} ${LIBTERMINFO}
LDADD=		-lcurses -lterminfo
SRCS=		cgram.c
MAN=		cgram.6
HIDEGAME=	hidegame
LINTFLAGS+=	-T

.include <bsd.prog.mk>
