#	$NetBSD: Makefile,v 1.3.6.1 2008/11/07 23:23:35 snj Exp $

.include <bsd.own.mk>

LIB=	Xinerama
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/man

SRCS=	Xinerama.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

MAN=	Xinerama.3
MLINKS=	Xinerama.3 XineramaQueryExtension.3 \
	Xinerama.3 XineramaQueryVersion.3 \
	Xinerama.3 XineramaIsActive.3 \
	Xinerama.3 XineramaQueryScreens.3

LIBDPLIBS=	Xext	${.CURDIR}/../libXext \
		X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
