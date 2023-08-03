#	$NetBSD: Makefile,v 1.15 2023/08/03 14:56:36 rin Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	find
SRCS=	find.c function.c ls.c main.c misc.c operator.c option.c

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
