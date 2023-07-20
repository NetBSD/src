#	$NetBSD: Makefile,v 1.8 2023/07/16 22:20:54 rjs Exp $

.include <bsd.own.mk>

.include "../mesa-which.mk"

LIB=		EGL
LIBISCXX=	yes

SHLIB_MAJOR=	0
SHLIB_MINOR=	0

#SUBDIR=		internal

# egl
.PATH:		${X11SRCDIR.Mesa}/src/egl/main
SRCS.egl= \
	eglapi.c \
	eglarray.c \
	eglconfig.c \
	eglcontext.c \
	eglcurrent.c \
	egldevice.c \
	egldisplay.c \
	eglglobals.c \
	eglimage.c \
	egllog.c \
	eglsurface.c \
	eglsync.c

.PATH:		${X11SRCDIR.Mesa}/src/egl/drivers/dri2
SRCS.drivers/dri2= \
	egl_dri2.c \
	platform_x11.c \
	platform_drm.c

# XXX notyet
#SRCS.drivers/dri2+= \
#	platform_x11_dri3.c

.PATH:		${X11SRCDIR.Mesa}/src/util
BUILDSYMLINKS+= ${X11SRCDIR.Mesa}/src/util/debug.c UTILdebug.c
SRCS.util=      \
	UTILdebug.c \
	os_file.c \
	u_process.c

.for file in ${SRCS.util}
CPPFLAGS.${file}+=		-I${X11SRCDIR.Mesa}/src/util \
				-I${X11SRCDIR.Mesa}/src/mesa \
				-I${X11SRCDIR.Mesa}/src \
				-I${X11SRCDIR.Mesa}/src/gallium/include
.endfor

.PATH:		${X11SRCDIR.Mesa}/include/EGL
INCS=	\
	egl.h \
	eglext.h \
	eglextchromium.h \
	eglmesaext.h \
	eglplatform.h
INCSDIR=${X11INCDIR}/EGL

SRCS+=	${SRCS.egl} ${SRCS.drivers/dri2} ${SRCS.util}

CPPFLAGS+= \
	-I${X11SRCDIR.Mesa}/include \
	-I${X11SRCDIR.Mesa}/include/drm-uapi \
	-I${X11SRCDIR.Mesa}/src/gbm/main \
	-I${X11SRCDIR.Mesa}/src/egl/main \
	-I${X11SRCDIR.Mesa}/src/gbm/backends/dri \
	-I${X11SRCDIR.Mesa}/src/loader \
	-I${X11SRCDIR.Mesa}/src \
	-I${DESTDIR}${X11INCDIR}/libdrm

EGL_DRIVER_INSTALL_DIR=${X11ROOTDIR}/lib
CPPFLAGS+= \
	-DDEFAULT_DRIVER_DIR=\"${X11USRLIBDIR}/modules/dri\" \
	-D_EGL_NATIVE_PLATFORM=_EGL_PLATFORM_X11 \
	-D_EGL_DRIVER_SEARCH_DIR=\"$(EGL_DRIVER_INSTALL_DIR)\" \
	-D_EGL_OS_UNIX=1

CPPFLAGS+=	${X11FLAGS.THREADLIB} \
		-DHAVE_X11_PLATFORM \
		-DHAVE_DRM_PLATFORM \
		-DHAVE_TIMESPEC_GET \
		-DHAVE_PTHREAD \
		-DHAVE_LIBDRM \
		-DHAVE_MINCORE

.include "../libloader.mk"

CFLAGS+=	-fvisibility=hidden -fno-strict-aliasing -fno-builtin-memcmp
CFLAGS+=	-pthread

# XXX missing: xcb-present
LIBDPLIBS=	Xext		${.CURDIR}/../libXext \
		X11		${.CURDIR}/../libX11/dynamic \
		Xfixes		${.CURDIR}/../libXfixes \
		Xdamage		${.CURDIR}/../libXdamage \
		Xdmcp		${.CURDIR}/../libXdmcp \
		glapi		${.CURDIR}/../libglapi${OLD_SUFFIX} \
		drm		${.CURDIR}/../libdrm \
		gbm		${.CURDIR}/../libgbm${OLD_SUFFIX} \
		xshmfence	${.CURDIR}/../libxshmfence \
		X11-xcb		${.CURDIR}/../libX11/libX11-xcb \
		xcb-dri2	${.CURDIR}/../libxcb/dri2 \
		xcb-glx		${.CURDIR}/../libxcb/glx \
		xcb-xfixes	${.CURDIR}/../libxcb/xfixes \
		xcb-sync	${.CURDIR}/../libxcb/sync \
		xcb		${.CURDIR}/../libxcb/libxcb \
		expat		${NETBSDSRCDIR}/external/mit/expat/lib/libexpat \
		m		${NETBSDSRCDIR}/lib/libm \
		pthread		${NETBSDSRCDIR}/lib/libpthread

MKLINT=no

PKGCONFIG=	egl
PKGDIST.egl=	${X11SRCDIR.Mesa}/../src/pkgconfig
.include "${.CURDIR}/../libGL/mesa-ver.mk"
PKGCONFIG_VERSION.egl=	${MESA_VER}

CWARNFLAGS.clang+=	-Wno-tautological-compare -Wno-format -Wno-constant-conversion
COPTS.platform_drm.c+=	-Wno-error=stack-protector

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
