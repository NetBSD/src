#	$NetBSD: Makefile,v 1.9 2020/11/07 18:12:16 nia Exp $

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
EGL=1
.else
EGL=0
.endif
CPPFLAGS+=	-DPLATFORM_HAS_EGL=${EGL}
CPPFLAGS+=	-DENABLE_EGL=${EGL}

INCSDIR=${X11INCDIR}/epoxy
INCS= \
	common.h \
	gl.h gl_generated.h \
	glx.h glx_generated.h \
	egl.h egl_generated.h

CPPFLAGS+=	-I${X11SRCDIR.${LIB}}/src
CPPFLAGS+=	-I${X11SRCDIR.${LIB}}/../include
CPPFLAGS+=	${X11FLAGS.CONNECTION}

PKGCONFIG=	epoxy
PKGCONFIG_SED_FLAGS= \
	-e "s,@DLOPEN_LIBS@,," \
	-e "s,@epoxy_has_glx@,1," \
	-e "s,@epoxy_has_egl@,${EGL}," \
	-e "s,@epoxy_has_wgl@,0,"

.if ${HAVE_XORG_GLAMOR} != "no"
PKGCONFIG_SED_FLAGS+= \
	-e "s,@GL_REQS@, gl egl,"
.else
PKGCONFIG_SED_FLAGS+= \
	-e "s,@GL_REQS@, gl,"
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>
