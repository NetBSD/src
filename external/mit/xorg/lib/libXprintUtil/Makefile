#	$NetBSD: Makefile,v 1.3 2008/11/06 22:28:26 veego Exp $

.include <bsd.own.mk>

LIB=	XprintUtil

.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/XprintUtil
SRCS=	xprintutil.c xprintutil_printtofile.c

INCS=	xprintutil.h
INCSDIR=${X11INCDIR}/X11/XprintUtil

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/XprintUtil

LIBDPLIBS=	Xp	${.CURDIR}/../libXp \
		X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
