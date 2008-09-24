#	$NetBSD: Makefile,v 1.2.2.3 2008/09/24 16:35:06 wrstuden Exp $

.include <bsd.own.mk>

LIB=	xkbui
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/extensions

SRCS=	XKBui.c

INCS=	XKBui.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=-I${DESTDIR}${X11INCDIR}/X11/extensions

PKGCONFIG=	xkbui

.include <bsd.x11.mk>
.include <bsd.lib.mk>
