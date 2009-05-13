#	$NetBSD: Makefile,v 1.9.36.1 2009/05/13 19:19:49 jym Exp $

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
