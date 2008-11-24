#	$NetBSD: Makefile,v 1.1 2008/11/24 02:03:52 cube Exp $

.include <bsd.own.mk>

LIB=		mesa
LIBISPRIVATE=	yes
NOSTATICLIB=	# defined

MESA_SRC_MODULES=	main math vbo tnl shader swrast slang

CPPFLAGS+=	${X11FLAGS.THREADLIB} \
		-DPTHREADS -D_NETBSD_SOURCE -DUSE_EXTERNAL_DXTN_LIB=1 \
		-DHAVE_ALIAS
CFLAGS+=	-pthread

.include "../../libOSMesa/libmesa.mk"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
