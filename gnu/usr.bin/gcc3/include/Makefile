#	$NetBSD: Makefile,v 1.1.2.1 2004/05/11 12:49:51 tron Exp $

NOOBJ=		# defined

.if ${MACHINE_ARCH} == "powerpc"
INCS=	altivec.h
.elif ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
INCS=	emmintrin.h mmintrin.h xmmintrin.h
.endif

INCSDIR=	/usr/include

.include <bsd.prog.mk>

.PATH: ${G_out_file:H}
