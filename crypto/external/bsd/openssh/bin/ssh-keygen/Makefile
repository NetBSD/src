#	$NetBSD: Makefile,v 1.6 2019/10/13 07:28:05 mrg Exp $

BINDIR=	/usr/bin

PROG=	ssh-keygen
SRCS=	ssh-keygen.c moduli.c sshsig.c

COPTS.ssh-keygen.c=	-Wno-pointer-sign

COPTS.ssh-keygen.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
