#	$NetBSD: Makefile,v 1.2.24.1 2021/06/02 14:46:20 cjep Exp $

.include <bsd.own.mk>

PROG=	xfindproxy

LDADD+=	${XLIBLDADD_XT}
DPADD+=	${XLIBDPADD_XT}

.PATH:	${X11SRCDIR.${PROG}}

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
