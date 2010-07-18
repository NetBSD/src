#	$NetBSD: Makefile,v 1.1 2010/07/18 07:01:39 mrg Exp $

XCBUTIL=	image

SRCS=	\
	xcb_image.c

.include "../xcb-util.mk"

LIBDPLIBS+=\
	xcb-shm	${.CURDIR}/../../libxcb/shm \
	xcb-aux	${.CURDIR}/../aux

CPPFLAGS+=	-I${X11SRCDIR.xcb-util}/aux
