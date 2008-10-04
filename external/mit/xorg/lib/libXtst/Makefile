#	$NetBSD: Makefile,v 1.3 2008/10/04 19:00:19 cube Exp $

.include <bsd.own.mk>

LIB=	Xtst
.PATH:	${X11SRCDIR.Xtst}/src ${X11SRCDIR.Xtst}/man

SRCS=	XTest.c XRecord.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

MAN=	XTestQueryExtension.3
MLINKS= XTestQueryExtension.3 XTestCompareCurrentCursorWithWindow.3 \
	XTestQueryExtension.3 XTestCompareCursorWithWindow.3 \
	XTestQueryExtension.3 XTestDiscard.3 \
	XTestQueryExtension.3 XTestFakeButtonEvent.3 \
	XTestQueryExtension.3 XTestFakeKeyEvent.3 \
	XTestQueryExtension.3 XTestFakeMotionEvent.3 \
	XTestQueryExtension.3 XTestFakeRelativeMotionEvent.3 \
	XTestQueryExtension.3 XTestGrabControl.3 \
	XTestQueryExtension.3 XTestSetGContextOfGC.3 \
	XTestQueryExtension.3 XTestSetVisualIDOfVisual.3

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
