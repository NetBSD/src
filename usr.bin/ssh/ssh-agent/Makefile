#	$NetBSD: Makefile,v 1.1 2000/07/25 16:32:25 thorpej Exp $

.include <bsd.own.mk>

PROG=	ssh-agent

SRCS=	ssh-agent.c log-client.c

.include <bsd.prog.mk>

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
