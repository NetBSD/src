#	$NetBSD: Makefile,v 1.2 2004/02/07 06:52:05 lukem Exp $

.include "../../../Makefile.serverlib"

LIB=	alpha

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/alpha
SRCS=	alphaFbs.c alphaIo.c alphaCursor.c alphaKbd.c alphaMouse.c \
	alphaKeyMap.c alphaInit.c alphaInEx.c

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/alpha/sfb
SRCS+=	alphaSFB.c sfbgc.c sfbbitblt.c sfbblt.c sfbwindow.c sfbsimpleblt.c \
	sfb32gc.c sfb32bitblt.c sfb32blt.c sfb32window.c

CPPFLAGS.sfb32gc.c=	-DPSZ=32
CPPFLAGS.sfb32bitblt.c=	-DPSZ=32
CPPFLAGS.sfb32blt.c=	-DPSZ=32
CPPFLAGS.sfb32window.c=	-DPSZ=32

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/alpha/tga
SRCS+=	alphaTGA.c tgagc.c tgabitblt.c tgablt.c tgawindow.c tgasimpleblt.c \
	tga32gc.c tga32bitblt.c tga32blt.c tga32window.c

CPPFLAGS.tga32gc.c=	-DPSZ=32
CPPFLAGS.tga32bitblt.c=	-DPSZ=32
CPPFLAGS.tga32blt.c=	-DPSZ=32
CPPFLAGS.tga32window.c=	-DPSZ=32

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/alpha \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/alpha/sfb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/alpha/tga \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/os \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${XSERVERINCDIR}

CPPFLAGS.alphaKbd.c=	-Wno-error

BUILDSYMLINKS+=	../../../mi/miinitext.c alphaInEx.c \
		sfbgc.c sfb32gc.c \
		sfbbitblt.c sfb32bitblt.c \
		sfbblt.c sfb32blt.c \
		sfbwindow.c sfb32window.c \
		tgagc.c tga32gc.c \
		tgabitblt.c tga32bitblt.c \
		tgablt.c tga32blt.c \
		tgawindow.c tga32window.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
