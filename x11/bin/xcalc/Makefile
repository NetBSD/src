#	$NetBSD: Makefile,v 1.3 2003/09/20 06:20:49 lukem Exp $

.include <bsd.own.mk>

PROG=	xcalc
SRCS=	actions.c math.c xcalc.c

APPDEFS=XCalc.ad XCalc-color.ad

BUILDSYMLINKS=	XCalc-col.ad XCalc-color.ad

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
