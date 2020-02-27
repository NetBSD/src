#	$NetBSD: Makefile,v 1.7 2020/02/27 00:24:39 christos Exp $

BINDIR=	/usr/bin

PROG=	ssh-keygen
SRCS=	ssh-keygen.c moduli.c sshsig.c ssh-sk-client.c

COPTS.ssh-keygen.c=	-Wno-pointer-sign

COPTS.ssh-keygen.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
