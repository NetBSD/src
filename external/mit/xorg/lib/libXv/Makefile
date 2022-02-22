#	$NetBSD: Makefile,v 1.5 2022/02/22 00:05:13 uwe Exp $

.include <bsd.own.mk>

LIB=	Xv
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/extensions
.PATH:	${X11SRCDIR.${LIB}}/man

SRCS=	Xv.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-DHAVE__XEATDATAWORDS

INCS=	Xvlib.h
INCSDIR=${X11INCDIR}/X11/extensions

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}

MAN +=		Xv.3
MAN +=		XvCreateImage.3
MAN +=		XvFreeAdaptorInfo.3
MAN +=		XvFreeEncodingInfo.3
MAN +=		XvGetPortAttribute.3
MAN +=		XvGetStill.3
MAN +=		XvGetVideo.3
MAN +=		XvGrabPort.3
MAN +=		XvListImageFormats.3
MAN +=		XvPortNotify.3
MAN +=		XvPutImage.3
MAN +=		XvPutStill.3
MAN +=		XvPutVideo.3
MAN +=		XvQueryAdaptors.3
MAN +=		XvQueryBestSize.3
MAN +=		XvQueryEncodings.3
MAN +=		XvQueryExtension.3
MAN +=		XvQueryPortAttributes.3
MAN +=		XvSelectPortNotify.3
MAN +=		XvSelectVideoNotify.3
MAN +=		XvSetPortAttribute.3
MAN +=		XvStopVideo.3
MAN +=		XvUngrabPort.3
MAN +=		XvVideoNotify.3

MLINKS +=	XvCreateImage.3	XvShmCreateImage.3
MLINKS +=	XvPutImage.3	XvShmPutImage.3

.include <bsd.x11.mk>
.include <bsd.lib.mk>
