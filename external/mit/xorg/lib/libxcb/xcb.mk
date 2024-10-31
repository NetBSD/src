#	$NetBSD: xcb.mk,v 1.6 2024/10/31 08:18:20 rillig Exp $

# define XCBEXT to something before including this

LIB=	xcb-${XCBEXT}

SRCS=	${XCBEXT}.c

CPPFLAGS+=	-I${X11SRCDIR.xcb}/src
CPPFLAGS+=	-I${.CURDIR}/../files

LINTFLAGS+=	-X 132	# int = long (mostly for pointer differences)
LINTFLAGS+=	-X 275	# cast discards 'const'
LINTFLAGS+=	-X 247	# cast between unrelated pointer types

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
