#	$NetBSD: Makefile,v 1.3 2008/10/12 20:20:49 hubertf Exp $

.include <bsd.own.mk>

LIB=	Xaw7
LIBOLD=	Xaw

.if ${MKPIC} != "no"
SYMLINKS+=	lib${LIB}.so.7 ${X11USRLIBDIR}/lib${LIBOLD}.so.7
.endif

.PATH:	${X11SRCDIR.Xaw}/src

SRCS=	Actions.c AllWidgets.c AsciiSink.c AsciiSrc.c AsciiText.c \
	Box.c Command.c Converters.c Dialog.c DisplayList.c Form.c \
	Grip.c Label.c List.c MenuButton.c MultiSink.c MultiSrc.c \
	OS.c Paned.c Panner.c Pixmap.c Porthole.c Repeater.c \
	Scrollbar.c Simple.c SimpleMenu.c Sme.c SmeBSB.c SmeLine.c \
	StripChart.c Text.c TextAction.c TextPop.c TextSink.c \
	TextSrc.c TextTr.c Tip.c Toggle.c Tree.c Vendor.c Viewport.c \
	XawI18n.c XawIm.c XawInit.c

CPPFLAGS+=	-DHAS_WCHAR_H -DHAS_WCTYPE_H -DNO_WIDEC_H -DHAVE_ISWALNUM
CPPFLAGS+=	-DPROJECT_ROOT=\"${X11ROOTDIR}\"

LIBDPLIBS=\
	Xmu	${.CURDIR}/../libXmu \
	Xt	${.CURDIR}/../libXt \
	Xpm	${.CURDIR}/../libXpm \
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

PKGCONFIG=	xaw7
PKGDIST=	${LIBOLD}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
