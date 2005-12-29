#	$NetBSD: Makefile,v 1.1 2005/12/29 15:52:21 tsutsui Exp $

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	ews4800mips

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/ews4800mips
SRCS=	ews4800mipsInit.c ews4800mipsScreen.c ews4800mipsIo.c ews4800mipsKbd.c \
	ews4800mipsMouse.c ews4800mipsFB.c ews4800mipsColormap.c \
	ews4800mipsKeymap.c ews4800mipsInEx.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/ews4800mips \
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

BUILDSYMLINKS=	../../../mi/miinitext.c ews4800mipsInEx.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
