#	$NetBSD: Makefile,v 1.3 2003/09/20 06:20:47 lukem Exp $

.include <bsd.own.mk>

PROG=	oclock
SRCS=	oclock.c Clock.c transform.c

APPDEFS=Clock-color.ad

BUILDSYMLINKS=	Clock-col.ad Clock-color.ad

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11 -lm
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
