#	$NetBSD: Makefile,v 1.1 2004/01/18 05:35:04 rtr Exp $

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	arm32vidc

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/arm32vidc
SRCS=	vidc.c rpccons.c wscons.c arm32vidcInEx.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/arm32vidc \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${XSERVERINCDIR}

BUILDSYMLINKS+=	../../../mi/miinitext.c arm32vidcInEx.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
