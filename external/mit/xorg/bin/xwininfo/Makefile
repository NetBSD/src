#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:04 haad Exp $

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
