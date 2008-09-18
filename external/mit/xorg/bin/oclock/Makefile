#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:27 wrstuden Exp $

.include <bsd.own.mk>

PROG=	oclock
SRCS=	oclock.c Clock.c transform.c

APPDEFS=Clock-color.ad

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11 -lm
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
