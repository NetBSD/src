#	$NetBSD: Makefile,v 1.12 2009/04/12 02:53:56 lukem Exp $

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
