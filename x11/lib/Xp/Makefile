#	$NetBSD: Makefile,v 1.2 2003/09/12 08:08:15 lukem Exp $

.include <bsd.own.mk>

LIB=	Xp
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XpAttr.c XpContext.c XpDoc.c XpExtUtil.c XpExtVer.c \
	XpGetData.c XpImageRes.c XpInput.c XpJob.c XpLocale.c \
	XpNotifyPdm.c XpPage.c XpPageDim.c XpPrinter.c XpPutData.c \
	XpScreens.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LDADD+=	-lXau -lXext -lX11
DPADD+=	${LIBXAU} ${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
