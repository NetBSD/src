#	$NetBSD: Makefile,v 1.1.1.1.4.1 2014/05/22 14:09:39 yamt Exp $

.include <bsd.own.mk>

PROG=	xstdcmap

CPPFLAGS+=      -DPACKAGE_STRING="\"xstdcmap 1.0.3\""

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
