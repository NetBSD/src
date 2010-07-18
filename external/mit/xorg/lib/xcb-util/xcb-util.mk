#	$NetBSD: xcb-util.mk,v 1.1 2010/07/18 07:01:38 mrg Exp $

# define XCBUTIL to something before including this

LIB=	xcb-${XCBUTIL}

CPPFLAGS+=	-I${X11SRCDIR.xcb-util}/${XCBUTIL}

LIBDPLIBS=\
	xcb	${.CURDIR}/../../libxcb/libxcb \
	Xau	${.CURDIR}/../../libXau \
	Xdmcp	${.CURDIR}/../../libXdmcp

SHLIB_MAJOR?=	0
SHLIB_MINOR?=	0

PKGCONFIG?=	xcb-${XCBUTIL}
PKGCONFIG_VERSION.${PKGCONFIG}=     0.3.6

# XXX totally fails
NOLINT=	# defined

.include <bsd.x11.mk>
.include <bsd.lib.mk>

.PATH: ${X11SRCDIR.xcb-util}/${XCBUTIL}
