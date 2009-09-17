#	$NetBSD: Makefile,v 1.1.1.1.8.1 2009/09/17 04:24:29 snj Exp $

.include <bsd.own.mk>

PROG=		xmore
APPDEFS=	XMore.ad

SRCS=	xmore.c

CPPFLAGS+=	-DXMORE

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} \
	${LIBICE} ${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
