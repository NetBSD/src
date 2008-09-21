# $NetBSD: dri.mk,v 1.2 2008/09/21 14:08:45 lukem Exp $

# XXX DRI_LIB_DEPS

LIBISMODULE=	yes

.include <bsd.own.mk>

SHLIB_MAJOR=	0

CPPFLAGS+=	-I${X11SRCDIR.MesaLib}/src/mesa/main \
		-I${X11SRCDIR.MesaLib}/src/mesa/glapi \
		-I${X11SRCDIR.MesaLib}/src/mesa/shader \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/common \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/${MODULE}/server \
		-I${X11SRCDIR.MesaLib}/src/mesa \
		-I${X11SRCDIR.MesaLib}/include \
		-I${DESTDIR}${X11INCDIR}/X11/drm \
		-I${DESTDIR}${X11INCDIR}/X11

CPPFLAGS+=	-D_NETBSD_SOURCE -DPTHREADS -DUSE_EXTERNAL_DXTN_LIB=1 \
		-DIN_DRI_DRIVER -DGLX_DIRECT_RENDERING \
		-DGLX_INDIRECT_RENDERING -DHAVE_ALIAS -DHAVE_POSIX_MEMALIGN

.PATH: ${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/${MODULE} ${DRI_EXTRA_PATHS}

# Common sources
.PATH:	${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/common \
	${X11SRCDIR.MesaLib}/src/mesa/drivers/common
SRCS+=	dri_util.c drirenderbuffer.c driverfuncs.c texmem.c utils.c vblank.c \
	xmlconfig.c

.include <bsd.x11.mk>

LIB=		${MODULE}_dri
LIBDIR=		${X11USRLIBDIR}/modules/dri

.include <bsd.lib.mk>
