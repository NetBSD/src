#	$NetBSD: Makefile,v 1.1 2001/01/14 05:22:34 itojun Exp $

.include <bsd.own.mk>

PROG=	ssh-keyscan
MAN=	ssh-keyscan.1

SRCS=	ssh-keyscan.c log-client.c

.include <bsd.prog.mk>

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
