#	$NetBSD: Makefile,v 1.5 2004/08/04 08:37:04 lukem Exp $

.include <bsd.own.mk>

LIB=	Xmuu
.PATH:	${X11SRCDIR.xc}/lib/Xmu

SRCS=	ClientWin.c CursorName.c DefErrMsg.c GetHost.c Lower.c

CPPFLAGS+=-DHAS_SNPRINTF

LIBDPLIBS=\
	Xt ../Xt \
	SM ../SM \
	ICE ../ICE \
	Xext ../Xext \
	X11 ../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
