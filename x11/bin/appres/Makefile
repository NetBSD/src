#	$NetBSD: Makefile,v 1.3 2010/03/22 00:23:59 mrg Exp $

.include <bsd.own.mk>

PROG=	appres

LDADD+=	-lXt -lSM -lICE -lXext -lX11 -lXau -lXdmcp
DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBXAU} ${LIBXDMCP}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
