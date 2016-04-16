#	$NetBSD: Makefile,v 1.5 2016/04/16 16:57:06 nakayama Exp $

.include <bsd.own.mk>

PROG=	smproxy
SRCS=	smproxy.c save.c

CPPFLAGS+=-DHAVE_MKSTEMP -DHAVE_ASPRINTF
CPPFLAGS+=	-DPACKAGE_STRING="\"smproxy 1.0.6\""

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
