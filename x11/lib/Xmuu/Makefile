#	$NetBSD: Makefile,v 1.7 2005/09/24 13:51:47 chs Exp $

.include <bsd.own.mk>

LIB=	Xmuu
.PATH:	${X11SRCDIR.xc}/lib/Xmu

SRCS=	ClientWin.c CursorName.c DefErrMsg.c GetHost.c Lower.c

CPPFLAGS+=-DHAS_SNPRINTF

LIBDPLIBS=\
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
