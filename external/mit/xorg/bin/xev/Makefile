#	$NetBSD: Makefile,v 1.4.24.1 2021/06/02 14:46:20 cjep Exp $

.include <bsd.own.mk>

PROG=	xev

LDADD+=	${XLIBLDADD_XRANDR}
DPADD+=	${XLIBDPADD_XRANDR}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
