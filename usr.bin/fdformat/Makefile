#	$NetBSD: Makefile,v 1.8 2002/12/10 16:07:28 fvdl Exp $

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
