#	$NetBSD: Makefile,v 1.1 2004/01/10 04:34:33 fredb Exp $

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	mac68k

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/mac68k
SRCS=	mac68kEvent.c mac68kInit.c mac68kKbd.c mac68kKeyMap.c mac68k.h \
	mac68kMouse.c mac68kMonoScreens.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

# trim fonts / extentions / XSERVERINCDIR if possible
CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/mac68k \
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

.include <bsd.x11.mk>
.include <bsd.lib.mk>
