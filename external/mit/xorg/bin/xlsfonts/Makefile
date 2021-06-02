#	$NetBSD: Makefile,v 1.3.24.1 2021/06/02 14:46:22 cjep Exp $

.include <bsd.own.mk>

PROG=	xlsfonts
SRCS=	xlsfonts.c dsimple.c

LDADD+=	${XLIBLDADD_XEXT}
DPADD+=	${XLIBDPADD_XEXT}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

COPTS.xlsfonts.c+=      -Wno-error

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
