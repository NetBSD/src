#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:03 haad Exp $

.include <bsd.own.mk>

PROG=	xpxthelloworld

.PATH:	${X11SRCDIR.xphelloworld}/${PROG}
SRCS=	xpxthelloworld.c

LDADD+=	-lXprintUtil -lXp -lXext -lX11 -lXaw -lXmu -lXt -lSM \
	-lICE -lXpm -lXp -lXext -lX11 -lm
DPADD+=	${LIBXPRINTUTIL} ${LIBXP} ${LIBXEXT} ${LIBX11} ${LIBXAW} \
	${LIBXMU} ${LIBXT} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXP} ${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
