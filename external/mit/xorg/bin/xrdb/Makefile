#	$NetBSD: Makefile,v 1.9 2022/09/19 18:46:06 christos Exp $

.include <bsd.own.mk>

PROG=	xrdb

CPPFLAGS+=-DCPP="\"/usr/bin/tradcpp -nostdinc\"" -DHAVE_MKSTEMP	# XXX
CPPFLAGS+=-DHAVE_ASPRINTF

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
