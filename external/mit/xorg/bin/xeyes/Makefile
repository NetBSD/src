#	$NetBSD: Makefile,v 1.2 2018/03/11 08:21:24 mrg Exp $

.include <bsd.own.mk>

PROG=	xeyes
SRCS=	xeyes.c Eyes.c transform.c

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11 -lm
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
