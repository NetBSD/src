#	$NetBSD: Makefile,v 1.17.36.1 2026/11/04 23:28:42 perseant Exp $
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
