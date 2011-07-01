#	$NetBSD: Makefile,v 1.3 2011/07/01 01:22:07 mrg Exp $

.include "../Makefile.inc"

.if ${MACHINE_CPU} == "powerpc"
INCS=	altivec.h
.elif ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
INCS= \
	abmintrin.h \
	ammintrin.h \
	avxintrin.h \
	bmmintrin.h \
	emmintrin.h \
	fma4intrin.h \
	ia32intrin.h \
	immintrin.h \
	lwpintrin.h \
	mm3dnow.h \
	mmintrin.h \
	nmmintrin.h \
	pmmintrin.h \
	popcntintrin.h \
	smmintrin.h \
	tmmintrin.h \
	wmmintrin.h \
	x86intrin.h \
	xmmintrin.h \
	xopintrin.h \
	mm_malloc.h

mm_malloc.h: ${DIST}/gcc/config/i386/pmm_malloc.h
	cp $> ${.TARGET}

CLEANFILES+=	mm_malloc.h
.endif

INCSDIR=	/usr/include/gcc-4.5

.include <bsd.prog.mk>

.PATH: ${G_out_file:H}
