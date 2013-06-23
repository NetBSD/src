#	$NetBSD: Makefile,v 1.2.12.1 2013/06/23 06:28:40 tls Exp $

.include <bsd.own.mk>

PROG=	oclock
SRCS=	oclock.c Clock.c transform.c

APPDEFS=Clock-color

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11 -lm
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
