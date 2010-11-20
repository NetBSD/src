#	$NetBSD: Makefile,v 1.2 2010/11/20 23:34:06 mrg Exp $

.include <bsd.own.mk>

PROG=	xsetroot

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11 -lXcursor
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBXCURSOR}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
