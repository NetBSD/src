#	$NetBSD: Makefile,v 1.1 2004/02/07 08:30:34 rtr Exp $

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	dreamcast

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dreamcast
SRCS=	dreamcastInit.c dreamcastScreen.c dreamcastIo.c dreamcastKbd.c \
	dreamcastMouse.c dreamcastFB.c dreamcastColormap.c \
	dreamcastKeymap.c dreamcastInEx.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/dreamcast \
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

BUILDSYMLINKS=	../../../mi/miinitext.c dreamcastInEx.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
