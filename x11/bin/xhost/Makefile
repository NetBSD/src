#	$NetBSD: Makefile,v 1.4 2004/04/29 05:58:54 lukem Exp $

.include <bsd.own.mk>

PROG=	xhost

CPPFLAGS+=${X11FLAGS.CONNECTION} -DBSD44SOCKETS

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
