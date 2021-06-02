#	$NetBSD: Makefile,v 1.3.36.1 2021/06/02 14:55:47 cjep Exp $

.include <bsd.own.mk>

PROG=	xclipboard

APPDEFS=XClipboard

LDADD+=	${XLIBLDADD_XAW}
DPADD+=	${XLIBDPADD_XAW}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
