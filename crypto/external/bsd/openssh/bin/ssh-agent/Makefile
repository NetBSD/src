#	$NetBSD: Makefile,v 1.2.48.1 2020/04/13 07:45:19 martin Exp $

BINDIR=/usr/bin

PROG=	ssh-agent
SRCS=	ssh-agent.c ssh-pkcs11-client.c

COPTS.ssh-agent.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
