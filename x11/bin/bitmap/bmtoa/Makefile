#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:02 lukem Exp $

NOMAN=	1

.include <bsd.own.mk>

PROG=	bmtoa

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/bitmap

.include <bsd.x11.mk>
.include <bsd.prog.mk>
