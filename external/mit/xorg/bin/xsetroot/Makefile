#	$NetBSD: Makefile,v 1.5.24.1 2021/06/06 20:30:47 cjep Exp $

.include <bsd.own.mk>

PROG=	xsetroot

LDADD+=	${XLIBLDADD_XMUU} ${XLIBLDADD_XCURSOR}
DPADD+=	${XLIBDPADD_XMUU} ${XLIBDPADD_XCURSOR}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
