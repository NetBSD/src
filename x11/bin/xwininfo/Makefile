#	$NetBSD: Makefile,v 1.3 2003/09/13 21:50:55 lukem Exp $

.include <bsd.own.mk>

PROG=	xwininfo
SRCS=	xwininfo.c dsimple.c

CPPFLAGS+=-I${X11SRCDIR.xc}/programs/xlsfonts
.PATH:	${X11SRCDIR.xc}/programs/xlsfonts

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
