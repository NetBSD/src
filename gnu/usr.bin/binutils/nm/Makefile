#	$NetBSD: Makefile,v 1.2 1998/07/29 19:18:27 thorpej Exp $

# XXX until our binutils is upgraded 
.if (${MACHINE_ARCH} != "powerpc")
PROG=	nm
SRCS=	nm.c
.endif

MAN=	nm.1

.include <bsd.prog.mk>
