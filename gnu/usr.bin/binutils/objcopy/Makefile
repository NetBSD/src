#	$NetBSD: Makefile,v 1.2 1998/07/29 19:18:27 thorpej Exp $

# XXX until our binutils is upgraded 
.if (${MACHINE_ARCH} != "powerpc")
PROG=	objcopy
SRCS=	objcopy.c not-strip.c
.endif

MAN=	objcopy.1

.include <bsd.prog.mk>
