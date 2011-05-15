#	$NetBSD: xcb-util.mk,v 1.2 2011/05/15 21:19:20 christos Exp $

# define XCBUTIL to something before including this

LIB=	xcb-${XCBUTIL}

CPPFLAGS+=	-I${X11SRCDIR.xcb-util}/${XCBUTIL}
CPPFLAGS+=	-DHAVE_VASPRINTF

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
