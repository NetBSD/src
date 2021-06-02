#	$NetBSD: Makefile,v 1.5.24.1 2021/06/02 15:31:33 cjep Exp $

.include <bsd.own.mk>

PROG=	appres

LDADD+=	${XLIBLDADD_XT}
DPADD+=	${XLIBDPADD_XT}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
