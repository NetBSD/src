# $NetBSD: dri.mk,v 1.3.4.3 2009/09/17 04:24:29 snj Exp $

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

CPPFLAGS+=	-Wno-stack-protector

.PATH: ${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/${MODULE} ${DRI_EXTRA_PATHS}

# Common sources
.PATH:	${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/common \
	${X11SRCDIR.MesaLib}/src/mesa/drivers/common
.if (${MODULE} == "swrast")
SRCS+=	driverfuncs.c utils.c
.else
SRCS+=	dri_util.c drirenderbuffer.c driverfuncs.c texmem.c
SRCS+=	utils.c vblank.c xmlconfig.c
.endif

.include <bsd.x11.mk>

LIB=		${MODULE}_dri
LIBDIR=		${X11USRLIBDIR}/modules/dri

LIBDPLIBS+= 	drm		${.CURDIR}/../../libdrm
LIBDPLIBS+=	expat		${.CURDIR}/../../expat
LIBDPLIBS+=	m		${NETBSDSRCDIR}/lib/libm
LIBDPLIBS+= 	mesa_dri	${.CURDIR}/../libmesa
# to find mesa_dri.so
LDFLAGS+=	-Wl,-rpath,${LIBDIR}

.include <bsd.lib.mk>
