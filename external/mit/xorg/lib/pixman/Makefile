#	$NetBSD: Makefile,v 1.17 2013/02/01 00:17:24 jmcneill Exp $

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
	pixman-x86.c			\
	pixman-mips.c			\
	pixman-arm.c			\
	pixman-ppc.c			\
	pixman-edge.c			\
	pixman-edge-accessors.c		\
	pixman-fast-path.c		\
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

# XXX

.if ${MACHINE_ARCH} == "i386"
# SSE2 requires a gcc >= 4.2
SRCS+= pixman-mmx.c
COPTS.pixman-mmx.c=	-mmmx -fvisibility=hidden
MKDEPFLAGS+=		-mmmx -fvisibility=hidden
.elif ${MACHINE_ARCH} == "x86_64"
SRCS+=	pixman-mmx.c
# XXX sse2 is broken with pixman 0.21.2; GCC generates unaligned accesses
#SRCS+=	pixman-sse2.c
COPTS.pixman-mmx.c=	-mmmx -fvisibility=hidden
#COPTS.pixman-sse2.c=	-msse2 -fvisibility=hidden
.endif

.if ${MACHINE_ARCH} == "earm"
# ARM SIMD
SRCS+=		pixman-arm-simd.c 		\
		pixman-arm-simd-asm.S
CPPFLAGS+=	-DUSE_ARM_SIMD
# ARM NEON
SRCS+=		pixman-arm-neon.c		\
		pixman-arm-neon-asm.S		\
		pixman-arm-neon-asm-bilinear.S
CPPFLAGS+=	-DUSE_ARM_NEON
# ARM iwMMX
SRCS+=		pixman-mmx.c
COPTS.pixman-mmx.c=	-mcpu=iwmmxt
CPPFLAGS+=	-DUSE_ARM_IWMMXT
.endif

# .if VMX
# SRCS+=	pixman-vmx.c
# .endif

INCS=	pixman.h pixman-version.h
INCSDIR=${X11INCDIR}/pixman-1

CPPFLAGS+=	-DHAVE_CONFIG_H \
		-I${X11SRCDIR.pixman}/../include \
		-I${X11SRCDIR.pixman}/../combine \
		-I${X11SRCDIR.pixman}/pixman

# With pixman 0.28.3, HAVE_PTHREAD_SETSPECIFIC is much faster than TLS
CPPFLAGS+=	-DHAVE_PTHREAD_SETSPECIFIC
#CPPFLAGS+=	-DTLS=__thread

LIBDPLIBS=	m	${.CURDIR}/../../../../../lib/libm

PKGCONFIG=	pixman-1
PKGDIST=	pixman

.include <bsd.x11.mk>
.include <bsd.lib.mk>
