#	$NetBSD: Makefile,v 1.1 2003/12/27 15:16:54 mrg Exp $

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	macppc

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/macppc
SRCS=	macppcFbs.c macppcIo.c macppcKeyMap.c macppcOFB.c macppc.h \
	macppcInit.c macppcKbd.c macppcMouse.c macppcInEx.c GetTime.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

# trim fonts / extentions / XSERVERINCDIR if possible
CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/macppc \
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

CPPFLAGS.macppcKbd.c=	 -Wno-error

BUILDSYMLINKS=	../../../mi/miinitext.c macppcInEx.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
