#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:50:49 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	Xxf86dga
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XF86DGA.c XF86DGA2.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
