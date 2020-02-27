#	$NetBSD: Makefile,v 1.4 2020/02/27 00:24:39 christos Exp $

BINDIR=/usr/bin

PROG=	ssh-agent
SRCS=	ssh-agent.c ssh-pkcs11-client.c ssh-sk-client.c

COPTS.ssh-agent.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
