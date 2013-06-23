#	$NetBSD: Makefile,v 1.1.1.1.28.1 2013/06/23 06:28:41 tls Exp $

.include <bsd.own.mk>

PROG=	xgamma

LDADD+=	-lXxf86vm -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXXF86VM} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
