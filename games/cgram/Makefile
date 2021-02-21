# $NetBSD: Makefile,v 1.3 2021/02/21 17:16:00 rillig Exp $

PROG=		cgram
DPADD=		${LIBCURSES} ${LIBTERMINFO}
LDADD=		-lcurses -lterminfo
SRCS=		cgram.c
MAN=		cgram.6
HIDEGAME=	hidegame
LINTFLAGS+=	-T
WARNS=		6

.include <bsd.prog.mk>
