#	$NetBSD: Makefile,v 1.3 2020/10/01 21:22:47 nia Exp $

.include <bsd.own.mk>

CPPFLAGS+=-DXRENDER

PROG=	xeyes
SRCS=	xeyes.c Eyes.c transform.c

LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lXrender -lX11 -lm
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
