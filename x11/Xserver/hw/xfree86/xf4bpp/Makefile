# $NetBSD: Makefile,v 1.1 2004/01/05 10:14:18 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	xf4bpp
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB}

SRCS=	emulOpStip.c emulRepAre.c emulTile.c mfbbres.c mfbbresd.c \
	mfbfillarc.c mfbhrzvert.c mfbimggblt.c mfbline.c mfbseg.c \
	mfbzerarc.c offscreen.c ppcArea.c ppcBStore.c ppcCReduce.c \
	ppcClip.c ppcCpArea.c ppcDepth.c ppcFillRct.c ppcGC.c ppcGetSp.c \
	ppcIO.c ppcImg.c ppcPixFS.c ppcPixmap.c ppcPntWin.c ppcPolyPnt.c \
	ppcPolyRec.c ppcQuery.c ppcRslvC.c ppcSetSp.c ppcWinFS.c \
	ppcWindow.c vgaBitBlt.c vgaGC.c vgaImages.c vgaSolid.c vgaStipple.c \
	vgamodule.c wm3.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf1bpp \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DXF86VGA16

CPPFLAGS.mfbseg.c=	-DPOLYSEGMENT

BUILDSYMLINKS=	mfbline.c mfbseg.c

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
