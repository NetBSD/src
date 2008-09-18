#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:33 wrstuden Exp $

.include <bsd.own.mk>

PROG=	xwininfo
SRCS=	xwininfo.c dsimple.c

XWININFODIR=	${X11SRCDIR.${PROG}}
CPPFLAGS+=-I${XWININFODIR} -I${XWININFODIR}/../include
.PATH:	${XWININFODIR}

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
