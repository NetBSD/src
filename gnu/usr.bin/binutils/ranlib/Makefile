#	$NetBSD: Makefile,v 1.2.2.2 1998/11/07 00:57:14 cgd Exp $

# XXX until our binutils is upgraded 
.if (${MACHINE_ARCH} != "powerpc")
PROG=	ranlib
SRCS=	ar.c is-ranlib.c
.endif

MAN=	ranlib.1

.include <bsd.prog.mk>
