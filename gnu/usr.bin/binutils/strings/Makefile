#	$NetBSD: Makefile,v 1.2.2.2 1998/11/07 00:57:19 cgd Exp $

# XXX until our binutils is upgraded 
.if (${MACHINE_ARCH} != "powerpc")
PROG=	strings
SRCS=	strings.c
.endif

MAN=	strings.1

.include <bsd.prog.mk>
