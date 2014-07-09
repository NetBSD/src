#	$NetBSD: Makefile,v 1.8 2014/07/09 20:22:56 riastradh Exp $

.include <bsd.own.mk>

LIB=		OSMesa

MESA_SRC_MODULES=	main math vbo tnl program swrast glapi common osmesa ss

.PATH:		${X11SRCDIR.MesaLib}/include/GL
INCS=		osmesa.h
INCSDIR=	${X11INCDIR}/GL

CPPFLAGS+=	${X11FLAGS.THREADLIB} \
		-DPTHREADS -DCHAN_BITS=32 -DDEFAULT_SOFTWARE_DEPTH_BITS=31 \
		-D_NETBSD_SOURCE -DUSE_EXTERNAL_DXTN_LIB=1 -DHAVE_ALIAS
CFLAGS+=	-pthread

CWARNFLAGS.clang+=	-Wno-format -Wno-constant-conversion

.include "libmesa.mk"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
