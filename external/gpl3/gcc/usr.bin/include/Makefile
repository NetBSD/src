#	$NetBSD: Makefile,v 1.10 2018/02/01 21:10:46 kamil Exp $

.include "../Makefile.inc"

SUBDIR=	sanitizer

.if exists(${.CURDIR}/arch/${GCC_MACHINE_ARCH}.mk)
.include "${.CURDIR}/arch/${GCC_MACHINE_ARCH}.mk"
.endif

INCS= ${G_EXTRA_HEADERS:T}

# Part of the standard headers, the rest of which we replace.
INCS+=	stdatomic.h

# XXX missed some how
.if ${GCC_MACHINE_ARCH} == "i386" || ${GCC_MACHINE_ARCH} == "x86_64"
INCS+=	mm_malloc.h

# XXX we should put this in gcc/config.gcc and regenerate the i386/x86
# mknative files
mm_malloc.h: ${DIST}/gcc/config/i386/pmm_malloc.h
	cp -f $> ${.TARGET}

CLEANFILES+=	mm_malloc.h
.endif

INCSDIR=	/usr/include/gcc-5

.include <bsd.subdir.mk>
.include <bsd.prog.mk>

.PATH: ${G_EXTRA_HEADERS:H:O:u}
