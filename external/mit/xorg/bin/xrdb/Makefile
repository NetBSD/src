#	$NetBSD: Makefile,v 1.2 2009/12/05 03:28:08 mrg Exp $

.include <bsd.own.mk>

PROG=	xrdb

CPPFLAGS+=-DCPP="\"/usr/bin/cpp\"" -DHAS_MKSTEMP	# XXX
CPPFLAGS+=-DCPP_ARGS="\"-traditional\""

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
