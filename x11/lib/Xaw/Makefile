#	$NetBSD: Makefile,v 1.3 2003/09/13 20:18:45 lukem Exp $

.include <bsd.own.mk>

LIB=	Xaw
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	Actions.c AllWidgets.c AsciiSink.c AsciiSrc.c AsciiText.c \
	Box.c Command.c Converters.c Dialog.c DisplayList.c Form.c \
	Grip.c Label.c List.c MenuButton.c MultiSink.c MultiSrc.c \
	OS.c Paned.c Panner.c Pixmap.c Porthole.c Repeater.c \
	Scrollbar.c Simple.c SimpleMenu.c Sme.c SmeBSB.c SmeLine.c \
	StripChart.c Text.c TextAction.c TextPop.c TextSink.c \
	TextSrc.c TextTr.c Tip.c Toggle.c Tree.c Vendor.c Viewport.c \
	XawI18n.c XawIm.c XawInit.c

MAN=	Xaw.3

INCS=	AllWidgets.h AsciiSink.h AsciiSinkP.h AsciiSrc.h AsciiSrcP.h \
	AsciiText.h AsciiTextP.h Box.h BoxP.h Cardinals.h Command.h \
	CommandP.h Dialog.h DialogP.h Form.h FormP.h Grip.h GripP.h \
	Label.h LabelP.h List.h ListP.h MenuButtoP.h MenuButton.h \
	MultiSink.h MultiSinkP.h MultiSrc.h MultiSrcP.h Paned.h \
	PanedP.h Panner.h PannerP.h Porthole.h PortholeP.h Repeater.h \
	RepeaterP.h Reports.h Scrollbar.h ScrollbarP.h Simple.h \
	SimpleMenP.h SimpleMenu.h SimpleP.h Sme.h SmeBSB.h SmeBSBP.h \
	SmeLine.h SmeLineP.h SmeP.h StripCharP.h StripChart.h \
	Template.c Template.h TemplateP.h Text.h TextP.h TextSink.h \
	TextSinkP.h TextSrc.h TextSrcP.h Tip.h TipP.h Toggle.h \
	ToggleP.h Tree.h TreeP.h VendorEP.h Viewport.h ViewportP.h \
	XawImP.h XawInit.h
INCSDIR=${X11INCDIR}/X11/Xaw

CPPFLAGS+=	-DHAS_WCHAR_H -DHAS_WCTYPE_H -DNO_WIDEC_H
CPPFLAGS+=	-DPROJECT_ROOT=\"${X11ROOTDIR}\"

LDADD+=	-lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXPM} ${LIBXEXT} ${LIBX11}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
