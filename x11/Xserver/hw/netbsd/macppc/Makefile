#	$NetBSD: Makefile,v 1.4.4.1 2004/12/07 11:00:33 rtr Exp $

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	macppc

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/macppc
SRCS=	macppcFbs.c macppcIo.c macppcKeyMap.c macppcOFB.c macppc.h \
	macppcInit.c macppcKbd.c macppcMouse.c macppcInEx.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/macppc \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/os \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-UXFree86LOADER -UXF86VIDMODE -UXFreeXDGA -UXF86MISC \
		-UXF86DRI -UXF86BIGFONT

CPPFLAGS.macppcKbd.c=	 -Wno-error

BUILDSYMLINKS=	../../../mi/miinitext.c macppcInEx.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
