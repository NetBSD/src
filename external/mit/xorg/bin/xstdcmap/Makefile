#	$NetBSD: Makefile,v 1.1.1.1.28.1 2013/06/23 06:28:42 tls Exp $

.include <bsd.own.mk>

PROG=	xstdcmap

CPPFLAGS+=      -DPACKAGE_STRING="\"xstdcmap 1.0.3\""

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
