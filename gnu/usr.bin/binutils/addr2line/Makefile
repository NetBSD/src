#	$NetBSD: Makefile,v 1.2.2.2 1998/11/07 00:56:51 cgd Exp $

# XXX until our binutils is upgraded 
.if (${MACHINE_ARCH} != "powerpc")
PROG=	addr2line
SRCS=	addr2line.c
.endif

MAN=	addr2line.1

.include <bsd.prog.mk>
