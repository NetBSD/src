#	$NetBSD: Makefile,v 1.2.50.1 2023/12/25 12:31:01 martin Exp $

BINDIR=/usr/bin

PROG=	ssh-agent
SRCS=	ssh-agent.c ssh-pkcs11-client.c

COPTS.ssh-agent.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.ssh-pkcs11-client.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
