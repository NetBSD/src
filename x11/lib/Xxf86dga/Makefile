#	$NetBSD: Makefile,v 1.3 2003/10/20 05:12:41 jdolecek Exp $

.include <bsd.own.mk>

LIB=	Xxf86dga
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XF86DGA.c XF86DGA2.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
