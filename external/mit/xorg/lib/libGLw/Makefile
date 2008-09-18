#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:35 wrstuden Exp $

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

.include <bsd.x11.mk>
.include <bsd.lib.mk>
