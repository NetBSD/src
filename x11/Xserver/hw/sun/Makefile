#	$NetBSD: Makefile,v 1.3 2003/12/23 04:43:04 rtr Exp $

.include "../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	sun

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/sun
SRCS=	sunInit.c sunCfb.c sunCfb24.c sunCursor.c sunFbs.c sunIo.c sunKbd.c \
	sunMfb.c sunMouse.c sunKeyMap.c sunGX.c sunInExMono.c sunInitExt.c \
	stubs.c sunInitMono.c sunInitMulti.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/sun \
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

CPPFLAGS.sunKbd.c=		-Wno-error
CPPFLAGS.sunGX.c=		-Wno-error
CPPFLAGS.sunInExMono.c=		-URENDER
CPPFLAGS.sunInitMono.c=		-DSUNMAXDEPTH=1
CPPFLAGS.sunInitMulti.c=	-DSUNMAXDEPTH=32

BUILDSYMLINKS=	../../mi/miinitext.c sunInExMono.c \
		../../mi/miinitext.c sunInitExt.c \
		../../Xi/stubs.c stubs.c \
		sunInit.c sunInitMono.c \
		sunInit.c sunInitMulti.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
