#	$NetBSD: Makefile,v 1.8 2005/09/24 13:51:47 chs Exp $

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
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
