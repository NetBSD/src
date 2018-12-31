#	$NetBSD: Makefile,v 1.4 2018/12/31 13:05:08 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		xkbstubs

.PATH:		${X11SRCDIR.xorg-server}/xkb
DDXSRCS=	ddxVT.c ddxPrivate.c ddxKillSrv.c

SRCS=		${DDXSRCS}

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/miext/damage \
		-I${X11SRCDIR.xorg-server}/render \
		-I${X11SRCDIR.xorg-server}/${LIB} \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/../include

CPPFLAGS+=	-DXKB_IN_SERVER -DHAVE_XKB_CONFIG_H

CLEANFILES+=		${BUILDOBJS}
realall: ${BUILDOBJS}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
