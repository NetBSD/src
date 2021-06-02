#	$NetBSD: Makefile,v 1.5.24.1 2021/06/02 14:46:24 cjep Exp $

.include <bsd.own.mk>

PROG=	xvinfo

LDADD+=	${XLIBLDADD_XV}
DPADD+=	${XLIBDPADD_XV}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

CWARNFLAGS.clang+=	-Wno-dangling-else

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
