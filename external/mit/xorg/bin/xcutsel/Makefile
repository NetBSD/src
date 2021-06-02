#	$NetBSD: Makefile,v 1.2.36.1 2021/06/02 14:46:20 cjep Exp $

.include <bsd.own.mk>

PROG=	xcutsel

LDADD+=	${XLIBLDADD_XAW}
DPADD+=	${XLIBDPADD_XAW}

.PATH:	${X11SRCDIR.xclipboard}
.PATH:	${X11SRCDIR.xclipboard}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
