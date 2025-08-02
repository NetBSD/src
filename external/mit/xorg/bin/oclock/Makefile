#	$NetBSD: Makefile,v 1.4.4.1 2025/08/02 05:50:38 perseant Exp $

.include <bsd.own.mk>

PROG=	oclock
SRCS=	oclock.c Clock.c transform.c

CPPFLAGS+=	-D_CONST_X_STRING

APPDEFS=Clock-color

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11 -lm
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
