#	$NetBSD: Makefile,v 1.3 2010/05/21 06:37:26 mrg Exp $

.include <bsd.own.mk>

PROG=		xvidtune
APPDEFS=	Xvidtune

SRCS=	xvidtune.c

LDADD+=	-lXxf86vm -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXXF86VM} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults

VENDOR_NAME=The Xorg Foundation

.include <bsd.x11.mk>
.include <bsd.prog.mk>
