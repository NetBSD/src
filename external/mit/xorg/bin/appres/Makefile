#	$NetBSD: Makefile,v 1.1.1.1.28.2 2014/08/20 00:00:08 tls Exp $

.include <bsd.own.mk>

PROG=	appres

CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include -DHAVE_CONFIG_H

LDADD+=	-lXt -lSM -lICE -lXext -lX11 -lxcb -lXdmcp -lXau
DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBXCB} ${LIBXDMCP} ${LIBXAU}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
