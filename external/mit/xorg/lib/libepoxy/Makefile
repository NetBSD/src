#	$NetBSD: Makefile,v 1.4 2019/04/18 10:10:33 mrg Exp $

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

.if ${HAVE_XORG_GLAMOR} != "no"
SRCS+= \
	dispatch_egl.c \
	egl_generated_dispatch.c
CPPFLAGS+=	-DPLATFORM_HAS_EGL=1
.endif

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
