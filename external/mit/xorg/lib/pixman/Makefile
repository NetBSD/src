#	$NetBSD: Makefile,v 1.6.2.3 2009/12/01 23:25:20 snj Exp $

NOLINT=	1	# defined

.include <bsd.own.mk>

LIB=		pixman-1

DIST=	${X11SRCDIR.pixman}/pixman
.PATH:	${DIST}

SRCS=	\
	pixman-access.c				\
	pixman-access-accessors.c		\
	pixman-cpu.c				\
	pixman-gradient-walker.c		\
	pixman-region16.c			\
	pixman-region32.c			\
	pixman-image.c				\
	pixman-implementation.c			\
	pixman-combine32.c			\
	pixman-combine64.c			\
	pixman-general.c			\
	pixman.c				\
	pixman-fast-path.c			\
	pixman-solid-fill.c			\
	pixman-conical-gradient.c		\
	pixman-linear-gradient.c		\
	pixman-radial-gradient.c		\
	pixman-bits-image.c			\
	pixman-utils.c				\
	pixman-edge.c				\
	pixman-edge-accessors.c			\
	pixman-trap.c				\
	pixman-timer.c				\
	pixman-matrix.c

# XXX

.if ${MACHINE_ARCH} == "i386"
# SSE2 requires a gcc >= 4.2
SRCS+= pixman-mmx.c
COPTS.pixman-mmx.c=	-mmmx -fvisibility=hidden
MKDEPFLAGS+=		-mmmx -fvisibility=hidden
.elif ${MACHINE_ARCH} == "x86_64"
SRCS+=	pixman-mmx.c
SRCS+=	pixman-sse2.c
COPTS.pixman-mmx.c=	-mmmx -fvisibility=hidden
COPTS.pixman-sse2.c=	-msse2 -fvisibility=hidden
MKDEPFLAGS+=		-mmmx -msse2 -fvisibility=hidden
.endif

# .if VMX
# SRCS+=	pixman-vmx.c
# .endif

INCS=	pixman.h pixman-version.h
INCSDIR=${X11INCDIR}/pixman-1

CPPFLAGS+=	-DPIXMAN_DISABLE_DEPRECATED -DHAVE_CONFIG_H \
		-I${X11SRCDIR.pixman}/../include \
		-I${X11SRCDIR.pixman}/../combine \
		-I${X11SRCDIR.pixman}/pixman

LIBDPLIBS=	m	${.CURDIR}/../../../../../lib/libm

PKGCONFIG=	pixman-1
PKGDIST=	pixman

.include <bsd.x11.mk>
.include <bsd.lib.mk>
