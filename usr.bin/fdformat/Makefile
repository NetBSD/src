#	$NetBSD: Makefile,v 1.13.6.1 2014/05/22 11:42:43 yamt Exp $

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
