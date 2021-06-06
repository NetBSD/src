#	$NetBSD: Makefile,v 1.3.36.1 2021/06/06 20:30:45 cjep Exp $

.include <bsd.own.mk>

PROG=	beforelight
SRCS=	b4light

APPDEFS=Beforelight

LDADD+=	${XLIBLDADD_XSS} ${XLIBLDADD_XAW}
DPADD+=	${XLIBDPADD_XSS} ${XLIBDPADD_XAW}

.PATH:	${X11SRCDIR.beforelight} ${X11SRCDIR.beforelight}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
