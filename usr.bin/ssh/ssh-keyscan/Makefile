#	$NetBSD: Makefile,v 1.3 2001/09/27 03:24:01 itojun Exp $

.include <bsd.own.mk>

PROG=	ssh-keyscan
MAN=	ssh-keyscan.1

SRCS=	ssh-keyscan.c

.include <bsd.prog.mk>

LDADD+=	-lcrypto -lz
DPADD+=	${LIBCRYPTO} ${LIBZ}
