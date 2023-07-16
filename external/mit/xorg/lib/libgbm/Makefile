#	$NetBSD: Makefile,v 1.9 2023/07/16 22:20:54 rjs Exp $

.include <bsd.own.mk>

.include "../mesa-which.mk"

LIB=		gbm

.PATH:	${X11SRCDIR.Mesa}/src/gbm/main
SRCS.main=	\
	backend.c \
	gbm.c

.PATH:	${X11SRCDIR.Mesa}/src/gbm/backends/dri
SRCS.dri=	\
	gbm_dri.c

BUILDSYMLINKS+= ${X11SRCDIR.Mesa}/src/util/debug.c UTILdebug.c
SRCS.util=      \
	UTILdebug.c \
	u_process.c
CPPFLAGS.UTILdebug.c+=          -I${X11SRCDIR.Mesa}/src/util \
				-I${X11SRCDIR.Mesa}/src/mesa \
				-I${X11SRCDIR.Mesa}/src \
				-I${X11SRCDIR.Mesa}/src/gallium/include

SRCS=	${SRCS.main} ${SRCS.dri} ${SRCS.util}

CPPFLAGS+=	-D_OS_UNIX=1
CPPFLAGS+=	-DHAVE_PTHREAD=1 -DHAVE_TIMESPEC_GET
CPPFLAGS+=	-DDEFAULT_BACKENDS_PATH=\"${X11USRLIBDIR}/modules/dri\"
CPPFLAGS+=	-DMODULEDIR=\"${X11USRLIBDIR}/gbm\"
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/include
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/loader
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/gbm/main
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/mapi
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm
CFLAGS+=	-fno-strict-aliasing -fexceptions

COPTS.gbm_dri.c+=	-Wno-error

.include "../libloader.mk"

.PATH:	${X11SRCDIR.Mesa}/src/gbm/main
.PATH:	${X11SRCDIR.Mesa}/src/gbm/backends/dri
INCS=		gbm.h
INCSDIR=	${X11INCDIR}

LIBDPLIBS=	glapi	${.CURDIR}/../libglapi${OLD_SUFFIX}
LIBDPLIBS+=	drm     ${.CURDIR}/../libdrm
LIBDPLIBS+=	expat	${NETBSDSRCDIR}/external/mit/expat/lib/libexpat

PKGCONFIG=	gbm
PKGDIST.gbm=	${X11SRCDIR.Mesa}/../src/pkgconfig
.include "${.CURDIR}/../libGL/mesa-ver.mk"
PKGCONFIG_VERSION.gbm=	${MESA_VER}

.include "../libdrm/Makefile.defines"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
