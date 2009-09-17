#	$NetBSD: Makefile,v 1.1.1.1.8.1 2009/09/17 04:24:29 snj Exp $

.include <bsd.own.mk>

PROG=	xwininfo
SRCS=	xwininfo.c clientwin.c dsimple.c

XWININFODIR=	${X11SRCDIR.${PROG}}
CPPFLAGS+=-I${XWININFODIR} -I${XWININFODIR}/../include
.PATH:	${XWININFODIR}

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
