#	$NetBSD: Makefile,v 1.3.2.2 1998/11/07 00:57:21 cgd Exp $

# XXX until our binutils is upgraded 
.if (${MACHINE_ARCH} != "powerpc")
PROG=	strip
SRCS=	objcopy.c is-strip.c
.endif

MAN=	strip.1

.include <bsd.prog.mk>
