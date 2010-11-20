#	$NetBSD: Makefile,v 1.3 2010/11/20 23:45:13 mrg Exp $

.include <bsd.own.mk>

PROG=	xwininfo
SRCS=	xwininfo.c clientwin.c dsimple.c

XWININFODIR=	${X11SRCDIR.${PROG}}
CPPFLAGS+=-I${XWININFODIR} -I${XWININFODIR}/../include
.PATH:	${XWININFODIR}

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11 -lxcb -lxcb-shape
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBXCB} ${LIBXCB_SHAPE}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
