#	$NetBSD: Makefile,v 1.1.4.1 2021/06/02 15:02:43 cjep Exp $

.include <bsd.own.mk>

PROG=	xcompmgr

LDADD+=	-lm -lXcomposite -lXdamage -lXfixes -lXrender
DPADD+=	${LIBM} ${LIBXCOMPOSITE} ${LIBXDAMAGE} ${LIBXFIXES} ${LIBXRENDER}
LDADD+=	${XLIBLDADD_XEXT}
DPADD+=	${XLIBDPADD_XEXT}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
