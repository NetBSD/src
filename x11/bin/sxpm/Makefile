#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:11 lukem Exp $

.include <bsd.own.mk>

PROG=	sxpm

LDADD+=	-lXpm -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXPM} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/extras/Xpm/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
