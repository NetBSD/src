#	$NetBSD: Makefile,v 1.8 2025/10/11 15:45:06 christos Exp $

BINDIR=/usr/bin

PROG=	ssh-agent
SRCS=	misc-agent.c ssh-agent.c ssh-pkcs11-client.c

COPTS.ssh-agent.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.ssh-pkcs11-client.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
