#	$NetBSD: Makefile,v 1.7 2005/09/24 13:51:47 chs Exp $

.include <bsd.own.mk>

LIB=	Xxf86dga
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XF86DGA.c XF86DGA2.c

LIBDPLIBS=\
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

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

CPPFLAGS+=	${X11FLAGS.THREADS}

CPPFLAGS.XF86DGA2.c=	-Wno-error	# gcc3

.include <bsd.x11.mk>
.include <bsd.lib.mk>
