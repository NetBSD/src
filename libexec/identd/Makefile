#	$NetBSD: Makefile,v 1.3 1996/08/30 17:41:29 thorpej Exp $

PROG=	identd
SRCS=	config.c identd.c netbsd.c parse.c proxy.c version.c
MAN=	identd.8

LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
