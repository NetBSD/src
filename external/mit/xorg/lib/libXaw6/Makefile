#	$NetBSD: Makefile,v 1.3.6.1 2009/05/13 18:53:26 jym Exp $

NOCHECKVER=	yes	# we want to install an older version ...
MKLINKLIB=	no

.include <bsd.own.mk>

LIB=	Xaw6
LIBOLD=	Xaw

.PATH:	${X11SRCDIR.Xaw}/src

SRCS=	Actions.c AllWidgets.c AsciiSink.c AsciiSrc.c AsciiText.c \
	Box.c Command.c Converters.c Dialog.c DisplayList.c Form.c \
	Grip.c Label.c List.c MenuButton.c MultiSink.c MultiSrc.c \
	OS.c Paned.c Panner.c Pixmap.c Porthole.c Repeater.c \
	Scrollbar.c Simple.c SimpleMenu.c Sme.c SmeBSB.c SmeLine.c \
	StripChart.c Text.c TextAction.c TextPop.c TextSink.c \
	TextSrc.c TextTr.c Toggle.c Tree.c Vendor.c Viewport.c \
	XawI18n.c XawIm.c XawInit.c

CPPFLAGS+=	-DHAS_WCHAR_H -DHAS_WCTYPE_H -DNO_WIDEC_H -DOLDXAW \
		-DHAVE_ISWALNUM

LIBDPLIBS=\
	Xmu	${.CURDIR}/../libXmu \
	Xt	${.CURDIR}/../libXt \
	SM	${.CURDIR}/../libSM \
	ICE	${.CURDIR}/../libICE \
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

PKGCONFIG=	xaw6
PKGDIST=	${LIBOLD}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
