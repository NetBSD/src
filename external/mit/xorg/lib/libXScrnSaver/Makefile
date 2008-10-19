#	$NetBSD: Makefile,v 1.3.4.2 2008/10/19 22:41:09 haad Exp $

.include <bsd.own.mk>

LIB=	Xss
.PATH:	${X11SRCDIR.XScrnSaver}/src ${X11SRCDIR.XScrnSaver}/man

SRCS=	XScrnSaver.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}
#CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}

LIBDPLIBS=	X11	${.CURDIR}/../libX11/dynamic

MAN=	Xss.3
MLINKS=	Xss.3 XScreenSaverAllocInfo.3 \
	Xss.3 XScreenSaverGetRegistered.3 \
	Xss.3 XScreenSaverQueryExtension.3 \
	Xss.3 XScreenSaverQueryInfo.3 \
	Xss.3 XScreenSaverQueryVersion.3 \
	Xss.3 XScreenSaverRegister.3 \
	Xss.3 XScreenSaverSelectInput.3 \
	Xss.3 XScreenSaverSetAttributes.3 \
	Xss.3 XScreenSaverSuspend.3 \
	Xss.3 XScreenSaverUnregister.3 \
	Xss.3 XScreenSaverUnsetAttributes.3

PKGDIST=	XScrnSaver

.include <bsd.x11.mk>
.include <bsd.lib.mk>
