#	$NetBSD: Makefile,v 1.2.40.1 2009/05/13 19:20:46 jym Exp $

.include "../../../Makefile.serverlib"

LIB=	dec

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dec
SRCS=	decInit.c decFbs.c decIo.c decCursor.c decKbd.c decKeyMap.c \
	decColormap.c decMouse.c decInEx.c

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dec/sfb
SRCS+=	decSFB.c sfbgc.c sfbbitblt.c sfbblt.c sfbwindow.c sfbsimpleblt.c \
	sfb32gc.c sfb32bitblt.c sfb32blt.c sfb32window.c

CPPFLAGS.sfb32gc.c=	-DPSZ=32
CPPFLAGS.sfb32bitblt.c=	-DPSZ=32
CPPFLAGS.sfb32blt.c=	-DPSZ=32
CPPFLAGS.sfb32window.c=	-DPSZ=32

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dec/px
SRCS+=	pxbitblt.c pxbresd.c pxgc.c pxgetsp.c pxline.c pxpacket.c \
	pxpolypnt.c pxsetsp.c pxwindow.c pxsegment.c pxfillrct.c \
	pxfillrctS.c pxfillrctSO.c pxfillsp.c pxPglyph.c \
	pxIglyph.c pximage.c pxmisc.c pxpushpxl.c pxfillarc.c \
	pxzerarc.c decPX.c

CPPFLAGS.pxsegment.c=	-D_POLYSEGMENT=1
CPPFLAGS.pxfillrctS.c=	-D_STIPPLE=1
CPPFLAGS.pxfillrctSO.c=	-D_STIPPLE=1 -D_OPAQUE=1
CPPFLAGS.pxPglyph.c=	-D_POLYGLYPH=1
CPPFLAGS.pxIglyph.c=	-D_IMAGEGLYPH=1

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dec/cfb
SRCS+=	decCfb.c

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dec/mfb
SRCS+=	decMfb.c

.if ${MACHINE_ARCH} == "alpha"
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dec/tga
SRCS+=	decTGA.c tgagc.c tgabitblt.c tgablt.c tgawindow.c tgasimpleblt.c \
	tga32gc.c tga32bitblt.c tga32blt.c tga32window.c

CPPFLAGS.tga32gc.c=	-DPSZ=32
CPPFLAGS.tga32bitblt.c=	-DPSZ=32
CPPFLAGS.tga32blt.c=	-DPSZ=32
CPPFLAGS.tga32window.c=	-DPSZ=32

BUILDSYMLINKS=	tgagc.c tga32gc.c \
		tgabitblt.c tga32bitblt.c \
		tgablt.c tga32blt.c \
		tgawindow.c tga32window.c

.endif

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dec \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dec/sfb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dec/px \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dec/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dec/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dec/tga \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/os \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${XSERVERINCDIR}

COPTS.decKbd.c=	-Wno-error

#CPPFLAGS+=	-UXFree86LOADER -UXF86VIDMODE -UXFreeXDGA -UXF86MISC \
#		-UXF86DRI -UXF86BIGFONT

BUILDSYMLINKS+=	../../../mi/miinitext.c decInEx.c \
		sfbgc.c sfb32gc.c \
		sfbbitblt.c sfb32bitblt.c \
		sfbblt.c sfb32blt.c \
		sfbwindow.c sfb32window.c \
		pxline.c pxsegment.c \
		pxfillrct.c pxfillrctS.c \
		pxfillrct.c pxfillrctSO.c \
		pxglyph.c pxPglyph.c \
		pxglyph.c pxIglyph.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
