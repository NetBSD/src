#	$NetBSD: Makefile,v 1.2.2.2 1998/11/07 00:57:17 cgd Exp $

# XXX until our binutils is upgraded 
.if (${MACHINE_ARCH} != "powerpc")
PROG=	size
SRCS=	size.c
.endif

MAN=	size.1

.include <bsd.prog.mk>
