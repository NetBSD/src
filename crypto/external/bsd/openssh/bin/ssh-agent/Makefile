#	$NetBSD: Makefile,v 1.7.2.1 2026/11/04 22:36:36 perseant Exp $

BINDIR=/usr/bin

PROG=	ssh-agent
SRCS=	misc-agent.c ssh-agent.c ssh-pkcs11-client.c

COPTS.ssh-agent.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.ssh-pkcs11-client.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
