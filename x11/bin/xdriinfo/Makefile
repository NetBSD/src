#	$NetBSD: Makefile,v 1.1.2.1 2004/11/15 09:12:45 rtr Exp $

.include <bsd.own.mk>

PROG=	xdriinfo

.PATH:	${X11SRCDIR.xc}/programs/${PROG}
SRCS=	xdriinfo.c

LDADD+=	-lGL -lXext -lX11 -lpthread
DPADD+=	${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
