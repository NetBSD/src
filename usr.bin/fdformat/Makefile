#	$NetBSD: Makefile,v 1.6 2001/04/12 02:31:52 briggs Exp $

.if (${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH} == "arm32" || \
     ${MACHINE} == "atari" || ${MACHINE} == "bebox" || \
     ${MACHINE_ARCH} == "i386" || \
     ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64")
PROG=	fdformat
.endif

MAN=	fdformat.1
.include <bsd.prog.mk>
