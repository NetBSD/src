#	$NetBSD: Makefile,v 1.3.24.1 2021/06/02 14:46:20 cjep Exp $

.include <bsd.own.mk>

PROG=	xcmsdb
SRCS=	xcmsdb.c loadData.c

LDADD+=	${XLIBLDADD_X11}
DPADD+=	${XLIBDPADD_X11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
