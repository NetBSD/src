#	$NetBSD: Makefile,v 1.3 2014/03/17 09:24:11 mrg Exp $

.include <bsd.own.mk>

PROG=	xauth
SRCS=	xauth.c gethost.c process.c parsedpy.c

CPPFLAGS+=		-DRETSIGTYPE=void -DPACKAGE_VERSION=\"1.0.8\"
CPPFLAGS.gethost.c=	${X11FLAGS.CONNECTION}
CPPFLAGS.parsedpy.c=	${X11FLAGS.CONNECTION}

LDADD+=	-lXau -lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXAU} ${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
