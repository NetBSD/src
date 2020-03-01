#	$NetBSD: Makefile,v 1.5 2020/03/01 20:59:53 christos Exp $

BINDIR=/usr/bin

PROG=	ssh-agent
SRCS=	ssh-agent.c ssh-pkcs11-client.c

COPTS.ssh-agent.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
