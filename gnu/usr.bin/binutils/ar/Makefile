#	$NetBSD: Makefile,v 1.2.2.2 1998/11/07 00:56:59 cgd Exp $

# XXX until our binutils is upgraded 
.if (${MACHINE_ARCH} != "powerpc")
PROG=	ar
SRCS=	ar.c not-ranlib.c
.endif

MAN=	ar.1

.include <bsd.prog.mk>
