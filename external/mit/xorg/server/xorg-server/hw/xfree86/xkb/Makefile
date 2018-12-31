# $NetBSD: Makefile,v 1.1 2018/12/31 11:14:05 mrg Exp $

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	xkb

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/${LIB}
SRCS=	xkbVT.c xkbPrivate.c xkbKillSrv.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/Xext \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include

.include <bsd.x11.mk>
.include <bsd.lib.mk>
