#	$NetBSD: Makefile,v 1.3.2.1 2004/04/29 05:46:27 jmc Exp $

.include <bsd.own.mk>

LIB=	XRes
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XRes.c

MAN=	XRes.3
MLINKS=	XRes.3 XResQueryClientPixmapBytes.3 \
	XRes.3 XResQueryClientResources.3 \
	XRes.3 XResQueryClients.3 \
	XRes.3 XResQueryExtension.3 \
	XRes.3 XResQueryVersion.3

CPPFLAGS+=	${X11FLAGS.THREADLIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
