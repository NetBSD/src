#	$NetBSD: Makefile,v 1.1 2004/08/01 18:52:03 uch Exp $

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	newsmips

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/newsmips
SRCS=	newsmipsInit.c newsmipsScreen.c newsmipsIo.c newsmipsKbd.c \
	newsmipsMouse.c newsmipsFB.c newsmipsColormap.c \
	newsmipsKeymap.c newsmipsInEx.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/newsmips \
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

BUILDSYMLINKS=	../../../mi/miinitext.c newsmipsInEx.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
