#	$NetBSD: Makefile,v 1.2 2022/10/29 21:06:15 mrg Exp $

.include <bsd.own.mk>

LIB=	Xpresent
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/man
.PATH:	${X11SRCDIR.${LIB}}/include/X11/extensions

SRCS=	Xpresent.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	Xrandr	${.CURDIR}/../libXrandr \
	Xfixes	${.CURDIR}/../libXfixes \
	X11	${.CURDIR}/../libX11/dynamic

INCS=	Xpresent.h

INCSDIR=${X11INCDIR}/X11/extensions

PKGDIST=	${LIB}

MAN=	Xpresent.3 \
	XPresentFreeInput.3 \
	XPresentNotifyMSC.3 \
	XPresentPixmap.3 \
	XPresentQueryCapabilities.3 \
	XPresentQueryExtension.3 \
	XPresentQueryVersion.3 \
	XPresentSelectInput.3 \
	XPresentVersion.3

.include <bsd.x11.mk>
.include <bsd.lib.mk>
