#	$NetBSD: Makefile,v 1.4.48.1 2012/04/17 00:09:56 yamt Exp $

.include "../../../Makefile.serverlib"

WARNS?=	2

LIB=	x68k

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/x68k
SRCS=	x68kInit.c x68kConfig.c x68kIo.c x68kMouse.c x68kKbd.c x68kKeyMap.c \
	x68kFb.c x68kGraph.c x68kText.c x68kInEx.c

# trim fonts / extensions / XSERVERINCDIR if possible
CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/x68k \
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

CPPFLAGS.x68kConfig.c=	 -DX11_LIBDIR=\"${X11ETCDIR}\"

BUILDSYMLINKS=	../../../mi/miinitext.c x68kInEx.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
