#	$NetBSD: Makefile,v 1.4 2004/03/14 20:11:24 wiz Exp $

.include "../../../Makefile.serverlib"

LIB=	hpc

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/hpc
SRCS=	hpc.h xf86Keymap.h atKeynames.h \
	hpcFB.c hpcIo.c hpcKeymap.c hpcScreen.c \
	hpcInit.c hpcKbd.c hpcMouse.c hpcColormap.c hpcInEx.c

# trim fonts / extensions / XSERVERINCDIR if possible
CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/hpc \
		-I${X11SRCDIR.xc}/programs/Xserver \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/os \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/include/extensions \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-UXFree86LOADER -UXF86VIDMODE -UXFreeXDGA -UXF86MISC \
		-UXF86DRI -UXF86BIGFONT

BUILDSYMLINKS=	../../../mi/miinitext.c hpcInEx.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
