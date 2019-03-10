#	$NetBSD: Makefile,v 1.1 2019/03/10 02:29:53 mrg Exp $

.include <bsd.own.mk>

LIB=		gbm

.PATH:	${X11SRCDIR.Mesa}/src/gbm/main
SRCS.main=	\
	backend.c \
	gbm.c

.PATH:	${X11SRCDIR.Mesa}/src/gbm/backends/dri
SRCS.dri=	\
	gbm_dri.c

SRCS=	${SRCS.main} ${SRCS.dri}

CPPFLAGS+=	-D_OS_UNIX=1
CPPFLAGS+=	-DHAVE_PTHREAD=1
CPPFLAGS+=	-DDEFAULT_DRIVER_DIR=\"${X11USRLIBDIR}/modules/dri\"
CPPFLAGS+=	-DMODULEDIR=\"${X11USRLIBDIR}/gbm\"
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/include
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/loader
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/gbm/main
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/mapi
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm
CFLAGS+=	-fno-strict-aliasing -fexceptions

COPTS.gbm_dri.c+=	-Wno-error

.include "../libloader.old.mk"

.PATH:	${X11SRCDIR.Mesa}/src/gbm/main
INCS=		gbm.h
INCSDIR=	${X11INCDIR}

LIBDPLIBS=      glapi	${.CURDIR}/../libglapi.old
LIBDPLIBS=      drm     ${.CURDIR}/../libdrm

PKGCONFIG=	gbm
PKGDIST.gbm=	${X11SRCDIR.Mesa}/src/gbm/main
.include "${.CURDIR}/../libGL.old/mesa-ver.mk"
PKGCONFIG_VERSION.gbm=	${MESA_VER}

.include "../libdrm/Makefile.defines"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
