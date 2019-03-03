#      $NetBSD: Makefile,v 1.3 2019/03/03 17:52:32 maya Exp $

.include <bsd.own.mk>

LIB=	drm_nouveau
.PATH: ${X11SRCDIR.drm}
.PATH: ${X11SRCDIR.drm}/nouveau
.PATH: ${X11SRCDIR.drm}/nouveau/nvif

SRCS=	nouveau.c \
	pushbuf.c \
	bufctx.c \
	abi16.c

COPTS.nouveau.c+=	${${ACTIVE_CC} == "clang":? -Wno-error=gnu-variable-sized-type-not-at-end :}

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 -I${X11SRCDIR.drm}

LIBDPLIBS=	drm	${.CURDIR}/../libdrm

PKGCONFIG=	libdrm_nouveau
PKGDIST=	drm

INCS=	cl0080.h \
	cl9097.h \
	class.h \
	if0002.h \
	if0003.h \
	ioctl.h \
	unpack.h

INCSDIR=${X11INCDIR}/libdrm/nouveau/nvif


.include "../libdrm/Makefile.defines"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
