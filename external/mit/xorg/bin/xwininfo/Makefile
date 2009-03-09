#	$NetBSD: Makefile,v 1.2 2009/03/09 07:07:19 mrg Exp $

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
