#	$NetBSD: Makefile,v 1.2 1998/07/29 19:18:26 thorpej Exp $

# XXX until our binutils is upgraded 
.if (${MACHINE_ARCH} != "powerpc")
PROG=	addr2line
SRCS=	addr2line.c
.endif

MAN=	addr2line.1

.include <bsd.prog.mk>
