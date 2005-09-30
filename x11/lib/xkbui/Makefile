#	$NetBSD: Makefile,v 1.3.8.1 2005/09/30 22:58:58 tron Exp $

.include <bsd.own.mk>

LIB=	xkbui
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XKBui.c

INCS=	XKBui.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=-I${X11SRCDIR.xc}/lib/xkbfile

LIBDPLIBS=\
	xkbfile	${.CURDIR}/../xkbfile \
	X11	${.CURDIR}/../X11/dynamic

LDADD+=		-lm
DPADD+=		${LIBM}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
