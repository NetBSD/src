#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:08 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	Xss
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XScrnSaver.c

#LM#	LDADD+=	-lXt -lSM -lICE -lXext -lX11
#LM#	DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
