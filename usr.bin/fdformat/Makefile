#	$NetBSD: Makefile,v 1.4 2000/08/09 11:38:47 mrg Exp $

.if (${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH} == "i386" || \
     ${MACHINE} == "atari" || ${MACHINE_ARCH} == "arm32" || \
     ${MACHINE_ARCH} == "sparc") || ${MACHINE_ARCH} == "sparc64")
PROG=	fdformat
.endif

MAN=	fdformat.1
.include <bsd.prog.mk>
