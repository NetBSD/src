#	$NetBSD: Makefile,v 1.4 2005/05/09 20:02:21 jmc Exp $

.include "../../Makefile.serverlib"

LIB=		dmx

.PATH:		${X11SRCDIR.xc}/programs/Xserver/hw/dmx
SRCS=		dmxinit.c dmxscrinit.c dmxshadow.c dmxcursor.c dmxinput.c \
		dmxdpms.c dmxgc.c dmxgcops.c dmxwindow.c dmxpixmap.c \
		dmxfont.c dmxcmap.c dmxvisual.c dmxlog.c dmxcb.c dmxprop.c \
		dmxstat.c dmxsync.c dmxextension.c dmxpict.c

.PATH:          ${X11SRCDIR.xc}/programs/Xserver/mi
SRCS+=		miinitext.c

.PATH:		${X11SRCDIR.xc}/doc/man/DMX
MAN=		DMX.3 DMXAddInput.3 DMXAddScreen.3 \
		DMXChangeDesktopAttributes.3 DMXChangeScreensAttributes.3 \
		DMXForceWindowCreation.3 DMXGetDesktopAttributes.3 \
		DMXGetInputAttributes.3 DMXGetInputCount.3 \
		DMXGetScreenAttributes.3 DMXGetScreenCount.3 \
		DMXGetWindowAttributes.3 DMXQueryExtension.3 \
		DMXQueryVersion.3 DMXRemoveInput.3 DMXRemoveScreen.3 DMXSync.3

MLINKS=         DMXAddInput.3 DMXAddBackendInput.3 \
		DMXAddInput.3 DMXAddConsoleInput.3


CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/dmx \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/os \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/miext/shadow \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DVENDOR_STRING="\"The XFree86 Project, Inc\"" \
		-DVENDOR_RELEASE="${XVERSION}" \
		-DOSNAME='"NetBSD/i386 2.99.16 [ELF]"' \
		-DOSVENDOR='"The NetBSD Foundation, Inc."'

CPPFLAGS+=	${X11FLAGS.OS_DEFINES} \
		${X11FLAGS.EXTENSION} \
		-DDMXEXT

CPPFLAGS+=	-UGLXEXT -UXV -UDBE -UXF86VIDMODE -UXF86MISC -UXFreeXDGA \
		-UMITSHM -UTOGCUP -UMITMISC -USCREENSAVER -URANDR -UFONTCACHE

CPPFLAGS.miinitext.c=	-UDPMSExtension

.include <bsd.x11.mk>
XVERSION!=	${PRINTX11VERSION}
.include <bsd.lib.mk>
SUBDIR=	dmxinput dmxconfig .WAIT bin
.include <bsd.subdir.mk>
