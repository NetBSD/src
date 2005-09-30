#	$NetBSD: Makefile,v 1.5.2.1 2005/09/30 22:58:58 tron Exp $

.include <bsd.own.mk>

LIB=	XRes
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XRes.c

LIBDPLIBS=\
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

MAN=	XRes.3
MLINKS=	XRes.3 XResQueryClientPixmapBytes.3 \
	XRes.3 XResQueryClientResources.3 \
	XRes.3 XResQueryClients.3 \
	XRes.3 XResQueryExtension.3 \
	XRes.3 XResQueryVersion.3

CPPFLAGS+=	${X11FLAGS.THREADS}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
