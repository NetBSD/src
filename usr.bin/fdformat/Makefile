#	$NetBSD: Makefile,v 1.7 2002/02/11 18:55:11 thorpej Exp $

.if (${MACHINE} == "acorn32" || \
     ${MACHINE_ARCH} == "alpha" || \
     ${MACHINE} == "cats" || \
     ${MACHINE} == "atari" || \
     ${MACHINE} == "bebox" || \
     ${MACHINE_ARCH} == "i386" || \
     ${MACHINE_ARCH} == "sparc" || \
     ${MACHINE_ARCH} == "sparc64")
PROG=	fdformat
.endif

MAN=	fdformat.1
.include <bsd.prog.mk>
