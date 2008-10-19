#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:40:56 haad Exp $

.include <bsd.own.mk>

PROG=	appres

LDADD+=	-lXt -lSM -lICE -lXext -lX11 -lXdmcp -lXau
DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBXDMCP} ${LIBXAU}

.PATH:	${X11SRCDIR.appres}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
