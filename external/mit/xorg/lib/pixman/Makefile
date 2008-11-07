#	$NetBSD: Makefile,v 1.6.2.1 2008/11/07 23:23:36 snj Exp $

NOLINT=	1	# defined

.include <bsd.own.mk>

LIB=		pixman-1

DIST=	${X11SRCDIR.pixman}/pixman
.PATH:	${DIST}

SRCS=	pixman-access.c pixman-region16.c pixman-access-accessors.c \
	pixman-region32.c pixman-image.c pixman-combine32.c \
	pixman-combine64.c pixman-compose.c pixman-compose-accessors.c \
	pixman-pict.c pixman-source.c pixman-transformed.c pixman-utils.c \
	pixman-transformed-accessors.c pixman-edge.c \
	pixman-edge-accessors.c pixman-trap.c pixman-compute-region.c \
	pixman-timer.c

# XXX

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SRCS+=	pixman-mmx.c
SRCS+=	pixman-sse2.c
MKDEPFLAGS+=		-msse -mmmx -fvisibility=hidden
COPTS.pixman-mmx.c=	-msse -mmmx -fvisibility=hidden
COPTS.pixman-sse2.c=	-msse -mmmx -fvisibility=hidden
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
