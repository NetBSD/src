# $NetBSD: Makefile,v 1.1.4.2 2019/06/10 22:02:49 christos Exp $

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
