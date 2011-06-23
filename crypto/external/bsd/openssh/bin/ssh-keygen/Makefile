#	$NetBSD: Makefile,v 1.2.4.1 2011/06/23 14:17:50 cherry Exp $

BINDIR=	/usr/bin

PROG=	ssh-keygen
SRCS=	ssh-keygen.c moduli.c

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.ssh-keygen.c=	-Wno-pointer-sign
.endif
