#	$NetBSD: Makefile,v 1.2 2006/05/17 03:29:45 mrg Exp $

.include "../Makefile.inc"

.if ${MACHINE_ARCH} == "powerpc"
INCS=	altivec.h
.elif ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
INCS=	emmintrin.h mmintrin.h xmmintrin.h mm_malloc.h

# We don't have posix_memalign, so use the gmm_malloc.h version.
mm_malloc.h: ${DIST}/gcc/config/i386/gmm_malloc.h
	cp $> ${.TARGET}

CLEANFILES+=	mm_malloc.h
.endif

INCSDIR=	/usr/include

.include <bsd.prog.mk>

.PATH: ${G_out_file:H}
