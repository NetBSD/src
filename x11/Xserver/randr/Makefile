#	$NetBSD: Makefile,v 1.2 2003/09/23 23:21:15 lukem Exp $

.include "../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		randr
.PATH:		${X11SRCDIR.xc}/programs/Xserver/randr

SRCS=		randr.c mirandr.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/kdrive \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions \
		-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/render

.include <bsd.x11.mk>
.include <bsd.lib.mk>
