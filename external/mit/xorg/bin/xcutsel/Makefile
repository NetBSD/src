#	$NetBSD: Makefile,v 1.3 2026/07/22 08:39:21 mrg Exp $

.include <bsd.own.mk>

PROG=	xcutsel

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xclipboard}
.PATH:	${X11SRCDIR.xclipboard}/man

XORG_PKG_VER_PROG=	xclipboard
.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
