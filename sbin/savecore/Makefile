#	$NetBSD: Makefile,v 1.24 2001/01/02 21:39:37 joda Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/17/94

PROG=	savecore
SRCS=	savecore.c zopen.c

MAN=	savecore.8

LDADD=	-lkvm -lz
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
