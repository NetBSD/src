#	$NetBSD: Makefile,v 1.1.2.1 2004/11/15 09:12:39 rtr Exp $

.include "../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		damage
.PATH:		${X11SRCDIR.xc}/programs/Xserver/damageext

SRCS=		damageext.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/xfixes \
		-I${X11SRCDIR.xc}/programs/Xserver/miext/damage \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/X11


.include <bsd.x11.mk>
.include <bsd.lib.mk>
