#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:38 wrstuden Exp $

.include <bsd.own.mk>

LIB=	Xtst
.PATH:	${X11SRCDIR.Xtst}/src

SRCS=	XTest.c XRecord.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
