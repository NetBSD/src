#	$NetBSD: Makefile,v 1.2 2001/03/19 20:03:26 itojun Exp $

.include <bsd.own.mk>

PROG=	ssh-keyscan
MAN=	ssh-keyscan.1

SRCS=	ssh-keyscan.c

.include <bsd.prog.mk>

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
