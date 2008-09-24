#	$NetBSD: Makefile,v 1.2.2.3 2008/09/24 16:35:05 wrstuden Exp $

.include <bsd.own.mk>

LIB=	Xtst
.PATH:	${X11SRCDIR.Xtst}/src

SRCS=	XTest.c XRecord.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
