#	$NetBSD: Makefile,v 1.2.2.1 2004/07/09 18:03:51 tron Exp $

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	dreamcast

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dreamcast
SRCS=	dreamcastInit.c dreamcastScreen.c dreamcastIo.c dreamcastKbd.c \
	dreamcastMouse.c dreamcastFB.c dreamcastColormap.c \
	dreamcastKeymap.c dreamcastInEx.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/dreamcast \
		-I${X11SRCDIR.xc}/programs/Xserver \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb32 \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/include/extensions \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-UXFree86LOADER -UXF86VIDMODE -UXFreeXDGA -UXF86MISC \
		-UXF86DRI -UXF86BIGFONT

BUILDSYMLINKS=	../../../mi/miinitext.c dreamcastInEx.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
