#	$NetBSD: Makefile,v 1.2 2010/11/21 02:33:39 mrg Exp $

.include <bsd.own.mk>

PROG=	beforelight
SRCS=	b4light

APPDEFS=Beforelight

LDADD+=	-lXss -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXSS} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.beforelight} ${X11SRCDIR.beforelight}/app-defaults

.include <bsd.x11.mk>
.include <bsd.prog.mk>
