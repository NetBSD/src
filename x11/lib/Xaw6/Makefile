#	$NetBSD: Makefile,v 1.3 2003/09/13 20:18:45 lukem Exp $

NOCHECKVER=	yes	# we want to install an older version ...
MKLINKLIB=	no

.include <bsd.own.mk>

LIB=	Xaw
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	Actions.c AllWidgets.c AsciiSink.c AsciiSrc.c AsciiText.c \
	Box.c Command.c Converters.c Dialog.c DisplayList.c Form.c \
	Grip.c Label.c List.c MenuButton.c MultiSink.c MultiSrc.c \
	OS.c Paned.c Panner.c Pixmap.c Porthole.c Repeater.c \
	Scrollbar.c Simple.c SimpleMenu.c Sme.c SmeBSB.c SmeLine.c \
	StripChart.c Text.c TextAction.c TextPop.c TextSink.c \
	TextSrc.c TextTr.c Toggle.c Tree.c Vendor.c Viewport.c \
	XawI18n.c XawIm.c XawInit.c

CPPFLAGS+=	-DHAS_WCHAR_H -DHAS_WCTYPE_H -DNO_WIDEC_H -DOLDXAW

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
