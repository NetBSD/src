#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:59 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	xkbui
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XKBui.c

INCS=	XKBui.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=-I${X11SRCDIR.xc}/lib/xkbfile

.include <bsd.x11.mk>
.include <bsd.lib.mk>
