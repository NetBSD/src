#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

.include <bsd.own.mk>

PROG=	xcutsel

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/xclipboard

.include <bsd.prog.mk>
.include <bsd.x11.mk>
