#	$NetBSD: Makefile,v 1.4 2003/09/20 06:20:51 lukem Exp $

.include <bsd.own.mk>

PROG=	xlsclients

BUILDSYMLINKS=	xlscli.man xlsclients.man

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
