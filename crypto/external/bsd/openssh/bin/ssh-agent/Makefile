#	$NetBSD: Makefile,v 1.5.8.1 2023/08/11 13:40:10 martin Exp $

BINDIR=/usr/bin

PROG=	ssh-agent
SRCS=	ssh-agent.c ssh-pkcs11-client.c

COPTS.ssh-agent.c+=	${GCC_NO_FORMAT_TRUNCATION}
COPTS.ssh-pkcs11-client.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
