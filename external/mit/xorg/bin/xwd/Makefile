#	$NetBSD: Makefile,v 1.5.10.1 2021/06/02 14:46:24 cjep Exp $

.include <bsd.own.mk>

PROG=	xwd
SRCS=	xwd.c clientwin.c dsimple.c list.c multiVis.c

CPPFLAGS+=-I${X11SRCDIR./xlsfonts} -DXKB
.PATH:	${X11SRCDIR./xlsfonts}

LDADD+=	${XLIBLDADD_XMUU}
DPADD+=	${XLIBDPADD_XMUU}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
