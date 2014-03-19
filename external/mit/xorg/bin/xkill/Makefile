#	$NetBSD: Makefile,v 1.2 2014/03/19 23:46:22 mrg Exp $

.include <bsd.own.mk>

PROG=	xkill

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

CPPFLAGS+=	-DPACKAGE_STRING=\"1.0.4\"

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
