#	$NetBSD: Makefile,v 1.3 1999/02/18 20:31:46 thorpej Exp $

.if (${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH} == "i386" || \
     ${MACHINE} == "atari" || ${MACHINE_ARCH} == "arm32" || \
     ${MACHINE_ARCH} == "sparc")
PROG=	fdformat
.endif

MAN=	fdformat.1
.include <bsd.prog.mk>
