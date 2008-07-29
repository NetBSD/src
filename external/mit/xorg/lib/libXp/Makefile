#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

LIB=	Xp
.PATH:	${X11SRCDIR.${LIB}}/src

SRCS=	XpAttr.c XpContext.c XpDoc.c XpExtUtil.c XpExtVer.c \
	XpGetData.c XpImageRes.c XpInput.c XpJob.c XpLocale.c \
	XpNotifyPdm.c XpPage.c XpPageDim.c XpPrinter.c XpPutData.c \
	XpScreens.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LIBDPLIBS=\
	Xau	${.CURDIR}/../libXau \
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
