#	$NetBSD: Makefile,v 1.4.36.1 2021/06/06 20:30:47 cjep Exp $

.include <bsd.own.mk>

PROG=		xvidtune
APPDEFS=	Xvidtune

SRCS=	xvidtune.c

LDADD+=	-lXxf86vm ${XLIBLDADD_XAW} # XXX
DPADD+=	${LIBXXF86VM} ${XLIBDPADD_XAW}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

VENDOR_NAME=The Xorg Foundation

.include <bsd.x11.mk>
.include <bsd.prog.mk>
