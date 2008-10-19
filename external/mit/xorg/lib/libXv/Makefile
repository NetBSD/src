#	$NetBSD: Makefile,v 1.3.4.2 2008/10/19 22:41:11 haad Exp $

.include <bsd.own.mk>

LIB=	Xv
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/extensions
.PATH:	${X11SRCDIR.${LIB}}/man

SRCS=	Xv.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

INCS=	Xvlib.h
INCSDIR=${X11INCDIR}/X11/extensions

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

MAN=	Xv.3 XvFreeAdaptorInfo.3 XvFreeEncodingInfo.3 XvGetPortAttribute.3 \
	XvGetStill.3 XvGetVideo.3 XvGrabPort.3 XvPortNotify.3 XvPutStill.3 \
	XvPutVideo.3 XvQueryAdaptors.3 XvQueryBestSize.3 XvQueryEncodings.3 \
	XvQueryExtension.3 XvSelectPortNotify.3 XvSelectVideoNotify.3 \
	XvSetPortAttribute.3 XvStopVideo.3 XvUngrabPort.3 XvVideoNotify.3

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
