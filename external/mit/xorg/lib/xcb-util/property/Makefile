#	$NetBSD: Makefile,v 1.1 2010/07/18 07:01:39 mrg Exp $

XCBUTIL=	property

SRCS=	property.c

SHLIB_MAJOR=	1

.include "../xcb-util.mk"

LIBDPLIBS+=\
	xcb-event	${.CURDIR}/../event

CPPFLAGS+=	-I${X11SRCDIR.xcb-util}/event
