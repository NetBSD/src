#	$NetBSD: Makefile,v 1.1.1.1.12.1 2011/04/07 06:54:58 snj Exp $

.include <bsd.own.mk>

PROG=	xrdb

CPPFLAGS+=-DCPP="\"/usr/bin/cpp\"" -DHAS_MKSTEMP	# XXX
CPPFLAGS+=-DCPP_ARGS="\"-traditional\""
CPPFLAGS+=-DHAVE_ASPRINTF

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
