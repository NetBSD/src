#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:52 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	Xxf86dga
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XF86DGA.c XF86DGA2.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
