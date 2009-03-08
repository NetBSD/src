#	$NetBSD: Makefile,v 1.11 2009/03/08 15:37:12 he Exp $

WARNS=3
.if (${MACHINE_ARCH} == "alpha" || \
     ${MACHINE_ARCH} == "arm" || \
     ${MACHINE_ARCH} == "armeb" || \
     ${MACHINE_ARCH} == "i386" || \
     ${MACHINE_ARCH} == "m68k" || \
     ${MACHINE_ARCH} == "powerpc" || \
     ${MACHINE_ARCH} == "sparc" || \
     ${MACHINE_ARCH} == "sparc64" || \
     ${MACHINE_ARCH} == "x86_64")
PROG=	fdformat
.endif

MAN=	fdformat.1
.include <bsd.prog.mk>
