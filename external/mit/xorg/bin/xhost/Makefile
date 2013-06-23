#	$NetBSD: Makefile,v 1.1.1.1.28.1 2013/06/23 06:28:41 tls Exp $

.include <bsd.own.mk>

PROG=	xhost

CPPFLAGS+=${X11FLAGS.CONNECTION} -DBSD44SOCKETS

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
