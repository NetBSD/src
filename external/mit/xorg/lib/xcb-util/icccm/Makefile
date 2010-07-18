#	$NetBSD: Makefile,v 1.1 2010/07/18 07:01:38 mrg Exp $

XCBUTIL=	icccm

SRCS=	icccm.c

SHLIB_MAJOR=	1

.include "../xcb-util.mk"

LIBDPLIBS+=\
	xcb-atom	${.CURDIR}/../atom \
	xcb-property	${.CURDIR}/../property \
	xcb-event	${.CURDIR}/../event

CPPFLAGS+=	-I${X11SRCDIR.xcb-util}/atom
CPPFLAGS+=	-I${X11SRCDIR.xcb-util}/property
CPPFLAGS+=	-I${X11SRCDIR.xcb-util}/event
