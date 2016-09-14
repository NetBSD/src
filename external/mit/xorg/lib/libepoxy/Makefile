#	$NetBSD: Makefile,v 1.1.2.2 2016/09/14 03:04:15 pgoyette Exp $

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

.include <bsd.x11.mk>
.include <bsd.lib.mk>
