#	$NetBSD: Makefile,v 1.3 2003/09/24 01:55:06 lukem Exp $

.include "../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		xpstubs
.PATH:		${X11SRCDIR.xc}/programs/Xserver/dix

SRCS=		xpstubs.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/lbx

.include <bsd.x11.mk>
.include <bsd.lib.mk>
