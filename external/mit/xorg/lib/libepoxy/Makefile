#	$NetBSD: Makefile,v 1.1.4.1 2017/04/21 16:52:58 bouyer Exp $

.include <bsd.own.mk>

LIB=	epoxy
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/epoxy
.PATH:	${X11SRCDIR.${LIB}}/../src

SRCS= \
	dispatch_common.c \
	dispatch_glx.c \
	gl_generated_dispatch.c \
	glx_generated_dispatch.c
SRCS.EGL= \
	dispatch_egl.c \
	egl_generated_dispatch.c

INCSDIR=${X11INCDIR}/epoxy
INCS= \
	gl.h gl_generated.h \
	glx.h glx_generated.h \
	egl.h egl_generated.h

CPPFLAGS+=	-I${X11SRCDIR.${LIB}}/src
CPPFLAGS+=	${X11FLAGS.CONNECTION}

PKGCONFIG=	epoxy
PKGCONFIG_SED_FLAGS= \
        -e "s,@DLOPEN_LIBS@,,"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
