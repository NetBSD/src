#	$NetBSD: Makefile,v 1.4.24.1 2021/06/02 14:46:21 cjep Exp $

.include <bsd.own.mk>

PROG=	xlsatoms

LDADD+=	${XLIBLDADD_XCB}
DPADD+=	${XLIBDPADD_XCB}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
