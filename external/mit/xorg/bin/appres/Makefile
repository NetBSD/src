#	$NetBSD: Makefile,v 1.1.1.1.28.1 2013/06/23 06:28:38 tls Exp $

.include <bsd.own.mk>

PROG=	appres

CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include -DHAVE_CONFIG_H

LDADD+=	-lXt -lSM -lICE -lXext -lX11 -lXdmcp -lXau
DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBXDMCP} ${LIBXAU}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
