#	$NetBSD: Makefile,v 1.5.8.1 2021/06/02 14:46:22 cjep Exp $

.include <bsd.own.mk>

PROG=		xmore
APPDEFS=	XMore

SRCS=	xmore.c

CPPFLAGS+=	-DXMORE

LDADD+=	${XLIBLDADD_XAW}
DPADD+=	${XLIBDPADD_XAW}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults ${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
