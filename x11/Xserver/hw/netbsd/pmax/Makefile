#	$NetBSD: Makefile,v 1.1 2004/01/10 05:26:14 rtr Exp $

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	pmax

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/pmax
SRCS=	pmax_init.c pmax_io.c lk201.c pmaxInEx.c GetTime.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/${LIB} \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-UXFree86LOADER -UXF86VIDMODE -UXFreeXDGA -UXF86MISC \
		-UXF86DRI -UXF86BIGFONT -DPSZ=8

BUILDSYMLINKS=	../../../mi/miinitext.c pmaxInEx.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
