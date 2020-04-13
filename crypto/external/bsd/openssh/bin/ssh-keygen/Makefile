#	$NetBSD: Makefile,v 1.4.34.1 2020/04/13 07:45:19 martin Exp $

BINDIR=	/usr/bin

PROG=	ssh-keygen
SRCS=	ssh-keygen.c moduli.c sshsig.c

COPTS.ssh-keygen.c=	-Wno-pointer-sign

COPTS.ssh-keygen.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
