#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:50:28 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	Xfontcache
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	FontCache.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
