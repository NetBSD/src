#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:47 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	Xfontcache
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	FontCache.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
