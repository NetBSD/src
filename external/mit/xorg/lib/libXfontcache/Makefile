#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

LIB=	Xfontcache
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/man

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

SRCS=	FontCache.c

MAN=	Xfontcache.3
MLINKS=	Xfontcache.3 FontCacheChangeCacheSettings.3 \
	Xfontcache.3 FontCacheGetCacheSettings.3 \
	Xfontcache.3 FontCacheGetCacheStatistics.3 \
	Xfontcache.3 FontCacheQueryExtension.3 \
	Xfontcache.3 FontCacheQueryVersion.3

#CPPFLAGS+=	${X11FLAGS.THREADS}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
