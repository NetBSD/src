#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:37 lukem Exp $

.include <bsd.own.mk>

PROG=	xstdcmap

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
