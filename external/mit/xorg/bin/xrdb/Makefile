#	$NetBSD: Makefile,v 1.8 2016/04/16 20:52:07 nakayama Exp $

.include <bsd.own.mk>

PROG=	xrdb

CPPFLAGS+=-DCPP="\"/usr/bin/cpp -traditional\"" -DHAVE_MKSTEMP	# XXX
CPPFLAGS+=-DHAVE_ASPRINTF

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
