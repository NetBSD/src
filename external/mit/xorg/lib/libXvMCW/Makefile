#	$NetBSD: Makefile,v 1.6 2023/06/03 09:09:08 lukem Exp $

.include <bsd.own.mk>

LIB=	XvMCW
.PATH:	${X11SRCDIR.XvMC}/wrapper

SRCS=	XvMCWrapper.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-DXVMC_CONFIGDIR=${X11ETCDIR}
CPPFLAGS+=	-DXVMC_SOVERSION=\".2.0\"
CPPFLAGS+=	-DXVMC_CLIENTSOVERSION=\".1.0\"
CPPFLAGS+=	-DXV_SOVERSION=\".2.0\"
CPPFLAGS+=	-I${X11SRCDIR.XvMC}/include

LIBDPLIBS=	Xext	${.CURDIR}/../libXext \
		X11	${.CURDIR}/../libX11/dynamic

COPTS.XvMCWrapper.c=	${CC_WNO_STRINGOP_OVERFLOW}

# XXX
#PKGDIST=	xvmc-wrapper
#.PATH:	${X11SRCDIR.XvMC}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
