#	$NetBSD: Makefile,v 1.3 2003/09/13 20:18:50 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	Xss
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XScrnSaver.c

LDADD+=	-lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
