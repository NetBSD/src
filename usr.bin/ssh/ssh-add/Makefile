#	$NetBSD: Makefile,v 1.2 2001/03/19 20:03:25 itojun Exp $

.include <bsd.own.mk>

PROG=	ssh-add

SRCS=	ssh-add.c

.include <bsd.prog.mk>

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
