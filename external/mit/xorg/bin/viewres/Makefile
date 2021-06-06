#	$NetBSD: Makefile,v 1.3.36.1 2021/06/06 20:30:45 cjep Exp $

.include <bsd.own.mk>

PROG=	viewres

APPDEFS=Viewres Viewres-color

LDADD+=	${XLIBLDADD_XAW} ${XLIBLDADD_XT}
DPADD+=	${XLIBDPADD_XAW} ${XLIBDPADD_XT}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
