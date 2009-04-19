#	$NetBSD: Makefile,v 1.2 2009/04/19 21:36:47 mrg Exp $

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
