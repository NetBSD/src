#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:00 haad Exp $

.include <bsd.own.mk>

PROG=	xcalc
SRCS=	actions.c math.c xcalc.c

APPDEFS=XCalc.ad XCalc-color.ad

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
