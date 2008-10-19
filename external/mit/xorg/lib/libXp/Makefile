#	$NetBSD: Makefile,v 1.3.4.2 2008/10/19 22:41:10 haad Exp $

.include <bsd.own.mk>

LIB=	Xp
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/man

SRCS=	XpAttr.c XpContext.c XpDoc.c XpExtUtil.c XpExtVer.c \
	XpGetData.c XpImageRes.c XpInput.c XpJob.c XpLocale.c \
	XpNotifyPdm.c XpPage.c XpPageDim.c XpPrinter.c XpPutData.c \
	XpScreens.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LIBDPLIBS=\
	Xau	${.CURDIR}/../libXau \
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

MAN=	libXp.3 XpCancelDoc.3 XpCancelJob.3 XpCancelPage.3 XpCreateContext.3 \
	XpDestroyContext.3 XpEndDoc.3 XpEndJob.3 XpEndPage.3 \
	XpFreePrinterList.3 XpGetAttributes.3 XpGetContext.3 \
	XpGetDocumentData.3 XpGetImageResolution.3 XpGetLocaleHinter.3 \
	XpGetOneAttribute.3 XpGetPageDimensions.3 XpGetPdmStartParams.3 \
	XpGetPrinterList.3 XpGetScreenOfContext.3 XpInputSelected.3 \
	XpPutDocumentData.3 XpQueryExtension.3 XpQueryScreens.3 \
	XpQueryVersion.3 XpRehashPrinterList.3 XpSelectInput.3 \
	XpSetAttributes.3 XpSetContext.3 XpSetImageResolution.3 \
	XpSetLocaleHinter.3 XpStartDoc.3 XpStartJob.3 XpStartPage.3

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
