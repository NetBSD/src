#	$NetBSD: Makefile,v 1.2.12.1 2013/06/23 06:28:40 tls Exp $

.include <bsd.own.mk>

PROG=	xcalc
SRCS=	actions.c math.c xcalc.c

APPDEFS=XCalc XCalc-color

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
