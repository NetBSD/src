#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:38 wrstuden Exp $

.include <bsd.own.mk>

LIB=	Xxf86dga
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/man

SRCS=	XF86DGA.c XF86DGA2.c

MAN=	XDGA.3
MLINKS=	XDGA.3 XF86DGA.3 \
	XDGA.3 XFree86-DGA.3 \
	XDGA.3 XDGAChangePixmapMode.3 \
	XDGA.3 XDGACloseFramebuffer.3 \
	XDGA.3 XDGACopyArea.3 \
	XDGA.3 XDGACopyTransparentArea.3 \
	XDGA.3 XDGACreateColormap.3 \
	XDGA.3 XDGAFillRectangle.3 \
	XDGA.3 XDGAGetViewportStatus.3 \
	XDGA.3 XDGAInstallColormap.3 \
	XDGA.3 XDGAKeyEventToXKeyEvent.3 \
	XDGA.3 XDGAOpenFramebuffer.3 \
	XDGA.3 XDGAQueryExtension.3 \
	XDGA.3 XDGAQueryModes.3 \
	XDGA.3 XDGAQueryVersion.3 \
	XDGA.3 XDGASelectInput.3 \
	XDGA.3 XDGASetClientVersion.3 \
	XDGA.3 XDGASetMode.3 \
	XDGA.3 XDGASetViewport.3 \
	XDGA.3 XDGASync.3

CPPFLAGS+=	${X11FLAGS.THREADLIB}

# XXX
CPPFLAGS.XF86DGA2.c=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
