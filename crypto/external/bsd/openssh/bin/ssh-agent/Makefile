#	$NetBSD: Makefile,v 1.3 2019/10/13 07:28:05 mrg Exp $

BINDIR=/usr/bin

PROG=	ssh-agent
SRCS=	ssh-agent.c ssh-pkcs11-client.c

COPTS.ssh-agent.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
