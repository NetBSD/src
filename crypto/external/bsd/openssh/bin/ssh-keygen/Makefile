#	$NetBSD: Makefile,v 1.2 2010/11/21 19:19:22 adam Exp $

BINDIR=	/usr/bin

PROG=	ssh-keygen
SRCS=	ssh-keygen.c moduli.c

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.ssh-keygen.c=	-Wno-pointer-sign
.endif
