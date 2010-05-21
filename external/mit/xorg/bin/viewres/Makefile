#	$NetBSD: Makefile,v 1.2 2010/05/21 06:04:36 mrg Exp $

.include <bsd.own.mk>

PROG=	viewres

APPDEFS=Viewres Viewres-color

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults

.include <bsd.x11.mk>
.include <bsd.prog.mk>
