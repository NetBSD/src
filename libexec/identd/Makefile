#	$NetBSD: Makefile,v 1.5 1997/10/08 09:07:20 mrg Exp $

PROG=	identd
SRCS=	config.c identd.c netbsd.c parse.c proxy.c version.c
MAN=	identd.8

LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
