#	$NetBSD: Makefile,v 1.3 2003/10/20 05:02:31 jdolecek Exp $

.include <bsd.own.mk>

LIB=	xkbui
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XKBui.c

INCS=	XKBui.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=-I${X11SRCDIR.xc}/lib/xkbfile

.include <bsd.x11.mk>
.include <bsd.lib.mk>
