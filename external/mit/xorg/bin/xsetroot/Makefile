#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:32 wrstuden Exp $

.include <bsd.own.mk>

PROG=	xsetroot

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
