#	$NetBSD: Makefile,v 1.6 2004/08/04 08:37:04 lukem Exp $

.include <bsd.own.mk>

LIB=	Xss
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XScrnSaver.c

MAN=	Xss.3
MLINKS=	Xss.3 XScreenSaverAllocInfo.3 \
	Xss.3 XScreenSaverGetRegistered.3 \
	Xss.3 XScreenSaverQueryExtension.3 \
	Xss.3 XScreenSaverQueryInfo.3 \
	Xss.3 XScreenSaverQueryVersion.3 \
	Xss.3 XScreenSaverRegister.3 \
	Xss.3 XScreenSaverSelectInput.3 \
	Xss.3 XScreenSaverSetAttributes.3 \
	Xss.3 XScreenSaverUnregister.3 \
	Xss.3 XScreenSaverUnsetAttributes.3

LIBDPLIBS=\
	Xt ../Xt \
	SM ../SM \
	ICE ../ICE \
	Xext ../Xext \
	X11 ../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
