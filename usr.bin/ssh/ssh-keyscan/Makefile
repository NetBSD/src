#	$NetBSD: Makefile,v 1.1.2.3 2001/12/10 23:53:12 he Exp $

.include <bsd.own.mk>

PROG=	ssh-keyscan
MAN=	ssh-keyscan.1

SRCS=	ssh-keyscan.c

.include <bsd.prog.mk>

LDADD+=	-lcrypto -lz
DPADD+=	${LIBCRYPTO} ${LIBZ}
