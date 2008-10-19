#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:40:59 haad Exp $

.include <bsd.own.mk>

PROG=	viewres

APPDEFS=Viewres.ad

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
