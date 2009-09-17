#	$NetBSD: Makefile,v 1.1.2.3 2009/09/17 04:24:42 snj Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		xkbstubs

.PATH:		${X11SRCDIR.xorg-server}/xkb
DDXSRCS=	ddxVT.c ddxPrivate.c ddxKillSrv.c

SRCS=		${DDXSRCS}

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/Xext/extmod \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/${LIB} \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/../include

CPPFLAGS+=	-DXKB_IN_SERVER -DHAVE_XKB_CONFIG_H

CLEANFILES+=		${BUILDOBJS}
realall: ${BUILDOBJS}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
