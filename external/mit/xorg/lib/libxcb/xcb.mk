#	$NetBSD: xcb.mk,v 1.4.16.1 2019/06/10 22:02:42 christos Exp $

# define XCBEXT to something before including this

LIB=	xcb-${XCBEXT}

SRCS=	${XCBEXT}.c

CPPFLAGS+=	-I${X11SRCDIR.xcb}/src
CPPFLAGS+=	-I${.CURDIR}/../files

LIBDPLIBS=\
	xcb	${.CURDIR}/../libxcb \
	Xau	${.CURDIR}/../../libXau \
	Xdmcp	${.CURDIR}/../../libXdmcp

SHLIB_MAJOR?=	0
SHLIB_MINOR?=	1

PKGCONFIG=		xcb-${XCBEXT}
PKGDIST.xcb-${XCBEXT}=	${X11SRCDIR.xcb}

.include <bsd.x11.mk>
.include <bsd.lib.mk>

.PATH: ${.CURDIR}/../files ${X11SRCDIR.xcb}
