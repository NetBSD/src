#	$NetBSD: Makefile,v 1.9 2004/04/23 14:50:00 christos Exp $

WARNS=3
.if (${MACHINE} == "acorn32" || \
     ${MACHINE_ARCH} == "alpha" || \
     ${MACHINE} == "cats" || \
     ${MACHINE} == "atari" || \
     ${MACHINE} == "bebox" || \
     ${MACHINE_ARCH} == "i386" || \
     ${MACHINE_ARCH} == "sparc" || \
     ${MACHINE_ARCH} == "sparc64" || \
     ${MACHINE_ARCH} == "x86_64")
PROG=	fdformat
.endif

MAN=	fdformat.1
.include <bsd.prog.mk>
