#	$NetBSD: Makefile,v 1.4.4.2 2004/11/21 10:01:26 rtr Exp $

.include "../../Makefile.serverlib"
.include "../../Makefile.servermod"

LIB=		xpstubs

.PATH:		${X11SRCDIR.xc}/programs/Xserver/dix
SRCS=		xpstubs.c

CPPFLAGS+= 	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/lbx \
		-I${XSERVERINCDIR}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
