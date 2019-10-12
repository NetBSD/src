#	$NetBSD: Makefile,v 1.5 2019/10/12 18:32:21 christos Exp $

BINDIR=	/usr/bin

PROG=	ssh-keygen
SRCS=	ssh-keygen.c moduli.c sshsig.c

COPTS.ssh-keygen.c=	-Wno-pointer-sign

.include <bsd.prog.mk>
