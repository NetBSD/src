#	$NetBSD: Makefile,v 1.15 2014/03/18 18:20:44 riastradh Exp $

.include <bsd.own.mk>

.if (${MACHINE_ARCH} == "alpha" || \
     ${MACHINE_CPU} == "arm" || \
     ${MACHINE_ARCH} == "i386" || \
     ${MACHINE_CPU} == "m68k" || \
     ${MACHINE_CPU} == "powerpc" || \
     ${MACHINE_ARCH} == "sparc" || \
     ${MACHINE_ARCH} == "sparc64" || \
     ${MACHINE_ARCH} == "x86_64")
PROG=	fdformat
.endif

MAN=	fdformat.1
.include <bsd.prog.mk>
