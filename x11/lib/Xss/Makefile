#	$NetBSD: Makefile,v 1.2 2003/09/12 08:08:19 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	Xss
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XScrnSaver.c

LDADD+=	-lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
