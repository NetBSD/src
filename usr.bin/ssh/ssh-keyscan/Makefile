#	$NetBSD: Makefile,v 1.1.2.2 2001/02/26 20:54:11 he Exp $

.include <bsd.own.mk>

PROG=	ssh-keyscan
MAN=	ssh-keyscan.1

SRCS=	ssh-keyscan.c log-client.c

.include <bsd.prog.mk>

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
