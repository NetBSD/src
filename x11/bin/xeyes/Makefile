#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:19 lukem Exp $

.include <bsd.own.mk>

PROG=	xeyes
SRCS=	xeyes.c Eyes.c transform.c

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11 -lm
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
