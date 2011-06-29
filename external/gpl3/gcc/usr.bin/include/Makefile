#	$NetBSD: Makefile,v 1.2 2011/06/29 08:37:10 mrg Exp $

.include "../Makefile.inc"

.if ${MACHINE_CPU} == "powerpc"
INCS=	altivec.h
.elif ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
INCS= \
	mmintrin.h \
	mm3dnow.h \
	xmmintrin.h \
	emmintrin.h \
	pmmintrin.h \
	tmmintrin.h \
	ammintrin.h \
	smmintrin.h \
	nmmintrin.h \
	bmmintrin.h \
	fma4intrin.h \
	wmmintrin.h \
	immintrin.h \
	x86intrin.h \
	avxintrin.h \
	xopintrin.h \
	ia32intrin.h \
	popcntintrin.h \
	lwpintrin.h \
	abmintrin.h

# XXX this comment is no longer true:
# We don't have posix_memalign, so use the gmm_malloc.h version.
mm_malloc.h: ${DIST}/gcc/config/i386/gmm_malloc.h
	cp $> ${.TARGET}

CLEANFILES+=	mm_malloc.h
.endif

INCSDIR=	/usr/include/gcc-4.5

.include <bsd.prog.mk>

.PATH: ${G_out_file:H}
