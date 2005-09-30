#	$NetBSD: Makefile,v 1.6.4.1 2005/09/30 22:58:57 tron Exp $

.include <bsd.own.mk>

LIB=	Xmuu
.PATH:	${X11SRCDIR.xc}/lib/Xmu

SRCS=	ClientWin.c CursorName.c DefErrMsg.c GetHost.c Lower.c

CPPFLAGS+=-DHAS_SNPRINTF

LIBDPLIBS=\
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
