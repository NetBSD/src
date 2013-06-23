#	$NetBSD: Makefile,v 1.2.12.1 2013/06/23 06:28:38 tls Exp $

.include <bsd.own.mk>

PROG=	beforelight
SRCS=	b4light

APPDEFS=Beforelight

LDADD+=	-lXss -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXSS} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.beforelight} ${X11SRCDIR.beforelight}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
