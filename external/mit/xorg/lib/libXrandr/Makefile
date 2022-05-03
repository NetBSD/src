#	$NetBSD: Makefile,v 1.7 2022/05/03 00:29:23 uwe Exp $

.include <bsd.own.mk>

LIB=	Xrandr
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/extensions
.PATH:	${X11SRCDIR.${LIB}}/man

SRCS=	Xrandr.c XrrConfig.c XrrCrtc.c XrrMode.c XrrOutput.c \
	XrrProperty.c XrrScreen.c XrrProvider.c XrrProviderProperty.c \
	XrrMonitor.c

INCS=	Xrandr.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions
CPPFLAGS+=	-DHAVE__XEATDATAWORDS

LIBDPLIBS=\
	Xrender	${.CURDIR}/../libXrender \
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}

CWARNFLAGS.clang+=	-Wno-pointer-sign


MAN = Xrandr.3

MLINKS += Xrandr.3 XRRConfigCurrentConfiguration.3
MLINKS += Xrandr.3 XRRConfigCurrentRate.3
MLINKS += Xrandr.3 XRRConfigRates.3
MLINKS += Xrandr.3 XRRConfigRotations.3
MLINKS += Xrandr.3 XRRConfigSizes.3
MLINKS += Xrandr.3 XRRConfigTimes.3
MLINKS += Xrandr.3 XRRFreeScreenConfigInfo.3
MLINKS += Xrandr.3 XRRGetScreenInfo.3
MLINKS += Xrandr.3 XRRQueryExtension.3
MLINKS += Xrandr.3 XRRQueryVersion.3
MLINKS += Xrandr.3 XRRRootToScreen.3
MLINKS += Xrandr.3 XRRSelectInput.3
MLINKS += Xrandr.3 XRRSetScreenConfig.3
MLINKS += Xrandr.3 XRRSetScreenConfigAndRate.3

.include <bsd.x11.mk>
.include <bsd.lib.mk>
