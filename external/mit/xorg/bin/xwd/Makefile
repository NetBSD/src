#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:04 haad Exp $

.include <bsd.own.mk>

PROG=	xwd
SRCS=	xwd.c dsimple.c list.c multiVis.c

CPPFLAGS+=-I${X11SRCDIR./xlsfonts}
.PATH:	${X11SRCDIR./xlsfonts}

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
