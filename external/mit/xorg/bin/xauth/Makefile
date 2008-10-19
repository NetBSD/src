#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:00 haad Exp $

.include <bsd.own.mk>

PROG=	xauth
SRCS=	xauth.c gethost.c process.c parsedpy.c

CPPFLAGS+=		-DRETSIGTYPE=void
CPPFLAGS.gethost.c=	${X11FLAGS.CONNECTION}
CPPFLAGS.parsedpy.c=	${X11FLAGS.CONNECTION}

LDADD+=	-lXau -lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXAU} ${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
