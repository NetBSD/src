#	$NetBSD: Makefile,v 1.2.2.3 2008/09/24 16:35:03 wrstuden Exp $

.include <bsd.own.mk>

LIB=	GLw
.PATH:	${X11SRCDIR.MesaLib}/src/glw

SRCS=	GLwDrawA.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LIBDPLIBS=	Xext	${.CURDIR}/../libXext \
		X11	${.CURDIR}/../libX11/dynamic \
		Xxf86vm	${.CURDIR}/../libXxf86vm \
		Xfixes	${.CURDIR}/../libXfixes \
		Xdamage	${.CURDIR}/../libXdamage \
		m	${.CURDIR}/../../../../../lib/libm \
		drm	${.CURDIR}/../libdrm \
		GL	${.CURDIR}/../libGL \
		ICE	${.CURDIR}/../libICE \
		SM	${.CURDIR}/../libSM \
		Xt	${.CURDIR}/../libXt

PKGCONFIG=	glw
PKGDIST.glw=	${X11SRCDIR.MesaLib}/src/glw
PKGCONFIG_VERSION.glw=	1.0.0

.include <bsd.x11.mk>
.include <bsd.lib.mk>
