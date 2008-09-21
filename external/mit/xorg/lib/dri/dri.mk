# $NetBSD: dri.mk,v 1.1 2008/09/21 00:46:59 cube Exp $

# XXX DRI_LIB_DEPS

NOLINT=		# don't build a lint library
NOPROFILE=	# don't build a profile library
NOPICINSTALL=	# don't intall a _pic.a library

.include <bsd.own.mk>

SHLIB_MAOR=	0
SHLIB_MINOR=	0

CPPFLAGS+=	-I${X11SRCDIR.MesaLib}/src/mesa/main \
		-I${X11SRCDIR.MesaLib}/src/mesa/glapi \
		-I${X11SRCDIR.MesaLib}/src/mesa/shader \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/common \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/${LIB}/server \
		-I${X11SRCDIR.MesaLib}/src/mesa \
		-I${X11SRCDIR.MesaLib}/include \
		-I${DESTDIR}${X11INCDIR}/X11/drm \
		-I${DESTDIR}${X11INCDIR}/X11

CPPFLAGS+=	-D_NETBSD_SOURCE -DPTHREADS -DUSE_EXTERNAL_DXTN_LIB=1 \
		-DIN_DRI_DRIVER -DGLX_DIRECT_RENDERING \
		-DGLX_INDIRECT_RENDERING -DHAVE_ALIAS -DHAVE_POSIX_MEMALIGN

.PATH: ${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/${LIB} ${DRI_EXTRA_PATHS}

# Common sources
.PATH:	${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/common \
	${X11SRCDIR.MesaLib}/src/mesa/drivers/common
SRCS+=	dri_util.c drirenderbuffer.c driverfuncs.c texmem.c utils.c vblank.c \
	xmlconfig.c

.if ${MKPIC} != "no"
.PRECIOUS:	${DESTDIR}${X11USRLIBDIR}/modules/dri/${LIB}_dri.so
libinstall::	${DESTDIR}${X11USRLIBDIR}/modules/dri/${LIB}_dri.so
.else
libinstall::
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>

${DESTDIR}${X11USRLIBDIR}/modules/dri/${LIB}_dri.so:	lib${LIB}.so.${SHLIB_FULLVERSION}
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    ${.ALLSRC} ${.TARGET}
