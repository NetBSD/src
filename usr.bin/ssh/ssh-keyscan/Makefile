#	$NetBSD: Makefile,v 1.3.2.1 2002/06/26 16:54:49 tv Exp $

.include <bsd.own.mk>

PROG=	ssh-keyscan

SRCS=	ssh-keyscan.c

MAN=	ssh-keyscan.1

.include <bsd.prog.mk>

LDADD+=	-lcrypto -lz
DPADD+=	${LIBCRYPTO} ${LIBZ}
