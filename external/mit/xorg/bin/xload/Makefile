#	$NetBSD: Makefile,v 1.5.36.1 2021/06/02 14:46:21 cjep Exp $

.include <bsd.own.mk>

PROG=	xload
SRCS=	xload.c get_load.c get_rload.c

CPPFLAGS+=${X11FLAGS.VERSION} -DHAVE_GETLOADAVG

APPDEFS=XLoad

LDADD+=	${XLIBLDADD_XAW}
DPADD+=	${XLIBDPADD_XAW}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
