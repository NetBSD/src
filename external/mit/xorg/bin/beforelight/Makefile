#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:40:56 haad Exp $

.include <bsd.own.mk>

PROG=	beforelight
SRCS=	b4light

APPDEFS=Beforelight.ad

LDADD+=	-lXss -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXSS} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.beforelight}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
