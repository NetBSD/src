#	$NetBSD: Makefile,v 1.4 2004/08/04 08:37:04 lukem Exp $

.include <bsd.own.mk>

LIB=	Xp
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XpAttr.c XpContext.c XpDoc.c XpExtUtil.c XpExtVer.c \
	XpGetData.c XpImageRes.c XpInput.c XpJob.c XpLocale.c \
	XpNotifyPdm.c XpPage.c XpPageDim.c XpPrinter.c XpPutData.c \
	XpScreens.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LIBDPLIBS=\
	Xau ../Xau \
	Xext ../Xext \
	X11 ../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
