#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:01 haad Exp $

.include <bsd.own.mk>

PROG=	xhost

CPPFLAGS+=${X11FLAGS.CONNECTION} -DBSD44SOCKETS

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
