#	$NetBSD: Makefile,v 1.4 2002/06/24 06:11:12 itojun Exp $

.include <bsd.own.mk>

PROG=	ssh-keyscan

SRCS=	ssh-keyscan.c

MAN=	ssh-keyscan.1

.include <bsd.prog.mk>

LDADD+=	-lcrypto -lz
DPADD+=	${LIBCRYPTO} ${LIBZ}
