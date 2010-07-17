#	$NetBSD: xcb.mk,v 1.1 2010/07/17 23:43:27 mrg Exp $

# define XCBEXT to something before including this

LIB=	xcb-${XCBEXT}

SRCS=	${XCBEXT}.c

CPPFLAGS+=	-I${X11SRCDIR.xcb}/src
CPPFLAGS+=	-I${.CURDIR}/../files

LIBDPLIBS=\
	xcb	${.CURDIR}/../libxcb \
	Xau	${.CURDIR}/../../libXau \
	Xdmcp	${.CURDIR}/../../libXdmcp

SHLIB_MAJOR=	0
SHLIB_MINOR=	0

PKGCONFIG=	xcb-${XCBEXT}
PKGCONFIG_VERSION.${PKGCONFIG}=     1.6

.include <bsd.x11.mk>
.include <bsd.lib.mk>

.PATH: ${.CURDIR}/../files ${X11SRCDIR.xcb}
