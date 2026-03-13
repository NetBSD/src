#	$NetBSD: Makefile,v 1.19 2026/03/13 20:55:59 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
WARNS=6

PROG=	sed
SRCS=	compile.c main.c misc.c process.c

.if (${HOSTPROG:U} == "")
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

.include <bsd.prog.mk>
