#	$NetBSD: Makefile,v 1.1 2011/01/19 16:31:24 jmcneill Exp $

.include <bsd.own.mk>

LIB=	XvMCW
.PATH:	${X11SRCDIR.XvMC}/src

SRCS=	XvMCWrapper.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-DXVMC_CONFIGDIR=${X11ETCDIR}
CPPFLAGS+=	-DXVMC_SOVERSION=\".2.0\"
CPPFLAGS+=	-DXVMC_CLIENTSOVERSION=\".1.0\"
CPPFLAGS+=	-DXV_SOVERSION=\".2.0\"

LIBDPLIBS=	Xext	${.CURDIR}/../libXext \
		X11	${.CURDIR}/../libX11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
