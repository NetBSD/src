#	$NetBSD: Makefile,v 1.7.4.1 2026/02/02 18:07:58 martin Exp $

BINDIR=/usr/bin

PROG=	ssh-agent
SRCS=	misc-agent.c ssh-agent.c ssh-pkcs11-client.c

COPTS.ssh-agent.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.ssh-pkcs11-client.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
