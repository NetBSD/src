#	$NetBSD: Makefile,v 1.2.2.3 2008/09/24 16:35:04 wrstuden Exp $

.include <bsd.own.mk>

LIB=	XprintUtil

.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/XprintUtil
SRCS=	xprintutil.c xprintutil_printtofile.c

INCS=	xprintutil.h
INCSDIR=${X11INCDIR}/X11/XprintUtil

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/XprintUtil

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
