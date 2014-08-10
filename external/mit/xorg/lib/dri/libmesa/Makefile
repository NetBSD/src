#	$NetBSD: Makefile,v 1.11.6.1 2014/08/10 06:50:56 tls Exp $

.include <bsd.own.mk>

_LIB_PREFIX=	lib
LIB=		mesa_dri
LIBISMODULE=	yes
LIBISCXX=	yes

SHLIB_MAJOR=	0

MESA_SRC_MODULES=	main math vbo tnl program swrast ss common

CPPFLAGS+=	\
	-I${X11SRCDIR.MesaLib}/include \
	-I${X11SRCDIR.MesaLib}/src/glsl \
	-I${X11SRCDIR.MesaLib}/src/mesa \
	-I${X11SRCDIR.MesaLib}/src/mapi

CPPFLAGS+=	${X11FLAGS.THREADLIB} \
		-DPTHREADS -D_NETBSD_SOURCE -DUSE_EXTERNAL_DXTN_LIB=1 \
		-DHAVE_ALIAS -DIN_DRI_DRIVER
#CPPFLAGS+=	-DGLX_DIRECT_RENDERING -DGLX_INDIRECT_RENDERING -DHAVE_POSIX_MEMALIGN
CFLAGS+=	-pthread

.include "../../libOSMesa/libmesa.mk"
.include "../../../tools/glsl/Makefile.glslsrcs"

SRCS+=  ${SRCS.glsl}

.include <bsd.x11.mk>

LIBDIR=		${X11USRLIBDIR}/modules/dri

#LIBDPLIBS+=	GL	${.CURDIR}/../../libGL

CWARNFLAGS.clang+=	-Wno-format -Wno-constant-conversion \
			-Wno-tautological-compare

.include <bsd.lib.mk>
