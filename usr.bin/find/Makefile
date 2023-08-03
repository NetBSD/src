#	$NetBSD: Makefile,v 1.14 2023/08/03 13:36:08 rin Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	find
SRCS=	find.c function.c ls.c main.c misc.c operator.c option.c

LDADD+=-lutil
DPADD+=${LIBUTIL}

COPTS.misc.c+=	${CC_WNO_USE_AFTER_FREE}

.include <bsd.prog.mk>
