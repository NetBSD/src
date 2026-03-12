#	$NetBSD: Makefile,v 1.18 2026/03/12 19:13:11 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
WARNS=6

PROG=	sed
SRCS=	compile.c main.c misc.c process.c
LDADD+= -lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
