#	$NetBSD: Makefile,v 1.4 1997/10/08 07:07:41 mrg Exp $

WARNS?=	1
PROG=	identd
SRCS=	config.c identd.c netbsd.c parse.c proxy.c version.c
MAN=	identd.8

LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
