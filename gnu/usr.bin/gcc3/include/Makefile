#	$NetBSD: Makefile,v 1.1 2003/12/05 18:56:11 matt Exp $

NOOBJ=		# defined

.if ${MACHINE_ARCH} == "powerpc"
INCS=	altivec.h
.elif ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
INCS=	mmintrin.h xmmintrin.h
.endif

INCSDIR=	/usr/include

.include <bsd.prog.mk>

.PATH: ${G_out_file:H}
