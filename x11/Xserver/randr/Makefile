#	$NetBSD: Makefile,v 1.4.4.1 2004/11/21 10:01:28 rtr Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		randr

.PATH:		${X11SRCDIR.xc}/programs/Xserver/randr
SRCS=		randr.c mirandr.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/kdrive \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/render

.include <bsd.x11.mk>
.include <bsd.lib.mk>
