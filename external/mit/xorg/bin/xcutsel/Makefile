#	$NetBSD: Makefile,v 1.1.1.1.28.1 2013/06/23 06:28:40 tls Exp $

.include <bsd.own.mk>

PROG=	xcutsel

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xclipboard}
.PATH:	${X11SRCDIR.xclipboard}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
