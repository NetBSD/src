#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:29 wrstuden Exp $

.include <bsd.own.mk>

PROG=	xeyes
SRCS=	xeyes.c Eyes.c transform.c

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11 -lm
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
