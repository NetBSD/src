#	$NetBSD: Makefile,v 1.2.2.2 1998/11/07 01:00:16 cgd Exp $

# XXX until our binutils is upgraded 
.if (${MACHINE_ARCH} != "powerpc")
PROG=	objdump
SRCS=	objdump.c prdbg.c
.endif

MAN=	objdump.1

.include <bsd.prog.mk>
