#	$NetBSD: Makefile,v 1.1.2.2 2001/12/10 23:53:34 he Exp $

.include <bsd.own.mk>

PROG=	ssh-keygen

SRCS=	ssh-keygen.c

.include <bsd.prog.mk>

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
