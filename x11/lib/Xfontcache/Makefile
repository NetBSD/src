#	$NetBSD: Makefile,v 1.4 2004/04/28 14:17:57 lukem Exp $

.include <bsd.own.mk>

LIB=	Xfontcache
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	FontCache.c

MAN=	Xfontcache.3
MLINKS=	Xfontcache.3 FontCacheChangeCacheSettings.3 \
	Xfontcache.3 FontCacheGetCacheSettings.3 \
	Xfontcache.3 FontCacheGetCacheStatistics.3 \
	Xfontcache.3 FontCacheQueryExtension.3 \
	Xfontcache.3 FontCacheQueryVersion.3

CPPFLAGS+=	${X11FLAGS.THREADLIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
