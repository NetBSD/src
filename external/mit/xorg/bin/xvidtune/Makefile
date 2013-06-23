#	$NetBSD: Makefile,v 1.3.12.1 2013/06/23 06:28:42 tls Exp $

.include <bsd.own.mk>

PROG=		xvidtune
APPDEFS=	Xvidtune

SRCS=	xvidtune.c

LDADD+=	-lXxf86vm -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXXF86VM} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

VENDOR_NAME=The Xorg Foundation

.include <bsd.x11.mk>
.include <bsd.prog.mk>
