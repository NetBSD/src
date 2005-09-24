#	$NetBSD: Makefile,v 1.4 2005/09/24 13:51:47 chs Exp $

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
