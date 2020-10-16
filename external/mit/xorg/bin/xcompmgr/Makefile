#	$NetBSD: Makefile,v 1.1 2020/10/16 08:28:17 nia Exp $

.include <bsd.own.mk>

PROG=	xcompmgr

LDADD+=	-lm -lXcomposite -lXdamage -lXfixes -lXrender
DPADD+=	${LIBM} ${LIBXCOMPOSITE} ${LIBXDAMAGE} ${LIBXFIXES} ${LIBXRENDER}
LDADD+=	-lX11 -lXext
DPADD+=	${LIBX11} ${LIBXEXT}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
