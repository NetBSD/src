#	$NetBSD: Makefile,v 1.5.8.2 2023/08/11 15:36:41 martin Exp $

BINDIR=/usr/bin

PROG=	ssh-agent
SRCS=	ssh-agent.c ssh-pkcs11-client.c

COPTS.ssh-agent.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.ssh-pkcs11-client.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
