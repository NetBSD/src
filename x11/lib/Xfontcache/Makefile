#	$NetBSD: Makefile,v 1.6 2005/09/24 13:51:47 chs Exp $

.include <bsd.own.mk>

LIB=	Xfontcache
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

LIBDPLIBS=\
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

SRCS=	FontCache.c

MAN=	Xfontcache.3
MLINKS=	Xfontcache.3 FontCacheChangeCacheSettings.3 \
	Xfontcache.3 FontCacheGetCacheSettings.3 \
	Xfontcache.3 FontCacheGetCacheStatistics.3 \
	Xfontcache.3 FontCacheQueryExtension.3 \
	Xfontcache.3 FontCacheQueryVersion.3

CPPFLAGS+=	${X11FLAGS.THREADS}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
