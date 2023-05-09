#	$NetBSD: Makefile,v 1.6 2023/05/09 22:25:26 christos Exp $

BINDIR=/usr/bin

PROG=	ssh-agent
SRCS=	ssh-agent.c ssh-pkcs11-client.c

COPTS.ssh-agent.c+=	${GCC_NO_FORMAT_TRUNCATION}
COPTS.ssh-pkcs11-client.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
