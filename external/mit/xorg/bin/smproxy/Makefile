#	$NetBSD: Makefile,v 1.6 2016/04/16 20:52:07 nakayama Exp $

.include <bsd.own.mk>

PROG=	smproxy
SRCS=	smproxy.c save.c

CPPFLAGS+=-DHAVE_MKSTEMP -DHAVE_ASPRINTF

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
