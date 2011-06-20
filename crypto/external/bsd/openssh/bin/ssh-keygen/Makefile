#	$NetBSD: Makefile,v 1.3 2011/06/20 07:43:56 mrg Exp $

BINDIR=	/usr/bin

PROG=	ssh-keygen
SRCS=	ssh-keygen.c moduli.c

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.ssh-keygen.c=	-Wno-pointer-sign
.endif
