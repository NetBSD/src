#	$NetBSD: Makefile,v 1.3 2013/05/31 08:02:00 mrg Exp $

.include <bsd.own.mk>

PROG=	xsetroot

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11 -lXcursor
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBXCURSOR}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
