#	$NetBSD: Makefile,v 1.1.6.1 2012/04/17 00:04:49 yamt Exp $

XCBUTIL=	property

SRCS=	property.c

SHLIB_MAJOR=	1

XCBUTIL_EXTRA_DPLIBS+=\
	xcb-event	${.CURDIR}/../event

.include "../xcb-util.mk"

CPPFLAGS+=	-I${X11SRCDIR.xcb-util}/event
