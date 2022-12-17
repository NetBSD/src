#	$NetBSD: Makefile,v 1.42 2022/12/17 09:46:21 mrg Exp $

NOLINT=	1	# defined

.include <bsd.own.mk>

LIB=		pixman-1

DIST=	${X11SRCDIR.pixman}/pixman
.PATH:	${DIST}

SRCS=					\
	pixman.c			\
	pixman-access.c			\
	pixman-access-accessors.c	\
	pixman-bits-image.c		\
	pixman-combine32.c		\
	pixman-combine-float.c		\
	pixman-conical-gradient.c	\
	pixman-edge.c			\
	pixman-edge-accessors.c		\
	pixman-fast-path.c		\
	pixman-filter.c			\
	pixman-glyph.c			\
	pixman-general.c		\
	pixman-gradient-walker.c	\
	pixman-image.c			\
	pixman-implementation.c		\
	pixman-linear-gradient.c	\
	pixman-matrix.c			\
	pixman-noop.c			\
	pixman-radial-gradient.c	\
	pixman-region16.c		\
	pixman-region32.c		\
	pixman-solid-fill.c		\
	pixman-timer.c			\
	pixman-trap.c			\
	pixman-utils.c

SRCS+=					\
	pixman-x86.c			\
	pixman-mips.c			\
	pixman-arm.c			\
	pixman-ppc.c

# XXX

# missing: HAVE_GCC_VECTOR_EXTENSIONS

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
. if ${MACHINE_ARCH} == "i386"
SRCS+= pixman-mmx.c
COPTS.pixman-mmx.c=	-mmmx -fvisibility=hidden
.  if defined(HAVE_LLVM)
COPTS.pixman-mmx.c+=	-Wno-error=attributes
.  endif
#CPPFLAGS+=		-DUSE_X86_MMX
MKDEPFLAGS+=		-mmmx
. endif
SRCS+=	pixman-sse2.c pixman-ssse3.c
COPTS.pixman-sse2.c=	-msse2 -fvisibility=hidden
COPTS.pixman-ssse3.c=	-msse3 -mssse3 -fvisibility=hidden
#CPPFLAGS+=		-DUSE_SSE2 -DUSE_SSSE3
MKDEPFLAGS+=		-msse2 -msse3 -mssse3 -fvisibility=hidden
.endif

.if ${MACHINE_ARCH} == "powerpc"
SRCS+=	pixman-vmx.c
COPTS.pixman-vmx.c=	-maltivec
#CPPFLAGS+=		-DUSE_VMX
MKDEPFLAGS+=		-maltivec
.endif

.if !empty(MACHINE_ARCH:Mearm*)
# ARM SIMD
SRCS+=		pixman-arm-simd.c 		\
		pixman-arm-simd-asm.S		\
		pixman-arm-simd-asm-scaled.S
# ARM NEON
SRCS+=		pixman-arm-neon.c		\
		pixman-arm-neon-asm.S		\
		pixman-arm-neon-asm-bilinear.S

AFLAGS+=	${${ACTIVE_CC} == "clang" :? -fno-integrated-as :}
# ARM iwMMX
#SRCS+=		pixman-mmx.c
#COPTS.pixman-mmx.c=	-mcpu=iwmmxt
.endif

.if !empty(MACHINE_ARCH:Maarch64*)
# XXX not yet; new pixman bad on arm64.
# ARM64 NEON
#SRCS+=		pixman-arm-neon.c			\
#		pixman-arma64-neon-asm.S		\
#		pixman-arma64-neon-asm-bilinear.S
#CPPFLAGS+=	-DUSE_ARM_A64_NEON
#
# XXX: not sure if needed?
#AFLAGS+=	${${ACTIVE_CC} == "clang" :? -fno-integrated-as :}
.endif

.if !empty(MACHINE_ARCH:Mmips*64el) && empty(COPTS:M-march=*)
# Loongson MMI
SRCS+=		pixman-mmx.c
#CPPFLAGS+=	-DUSE_LOONGSON_MMI
# SIMD instructions use floatpoint registers so we need to enable their use
COPTS.pixman-mmx.c=	-march=loongson2f -mhard-float -Wa,-mhard-float
.endif

.if ${MACHINE_CPU} == "sh3"
# XXX pixman-fast-path.c:1204:1: internal compiler error: Segmentation fault
COPTS.pixman-fast-path.c+=	-O1
.endif

.include "Makefile.cflags"

INCS=	pixman.h pixman-version.h
INCSDIR=${X11INCDIR}/pixman-1

# With pixman 0.28.3, HAVE_PTHREAD_SETSPECIFIC is much faster than TLS
CPPFLAGS+=	-DHAVE_PTHREAD_SETSPECIFIC
#CPPFLAGS+=	-DTLS=__thread

LIBDPLIBS=	m	${.CURDIR}/../../../../../lib/libm

PKGCONFIG=	pixman-1
PKGDIST=	pixman

.include <bsd.x11.mk>
.include <bsd.lib.mk>
