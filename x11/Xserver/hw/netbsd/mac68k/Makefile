#	$NetBSD: Makefile,v 1.3 2004/03/14 20:11:24 wiz Exp $

.include "../../../Makefile.serverlib"

LIB=	mac68k

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/mac68k
SRCS=	mac68kEvent.c mac68kInit.c mac68kKbd.c mac68kKeyMap.c mac68k.h \
	mac68kMouse.c mac68kMonoScreens.c

# trim fonts / extensions / XSERVERINCDIR if possible
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
