#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

LIB=	XprintUtil

.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/XprintUtil
SRCS=	xprintutil.c xprintutil_printtofile.c

INCS=	xprintutil.h
INCSDIR=${X11INCDIR}/X11/XprintUtil

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/XprintUtil

.include <bsd.x11.mk>
.include <bsd.lib.mk>
