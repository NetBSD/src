#	$NetBSD: Makefile,v 1.2 1998/07/29 19:18:28 thorpej Exp $

# XXX until our binutils is upgraded 
.if (${MACHINE_ARCH} != "powerpc")
PROG=	strip
SRCS=	objcopy.c is-strip.c
.endif

MAN=	strip.1

.include <bsd.prog.mk>
