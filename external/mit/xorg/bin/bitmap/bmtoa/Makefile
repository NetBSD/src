#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:25 wrstuden Exp $

NOMAN=	1

.include <bsd.own.mk>

PROG=	bmtoa

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.bitmap}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
