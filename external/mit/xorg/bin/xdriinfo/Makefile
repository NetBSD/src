#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:00 haad Exp $

.include <bsd.own.mk>

PROG=	xdriinfo

SRCS=	xdriinfo.c

LDADD+=	-lGL -lXext -lX11 -lpthread
DPADD+=	${LIBGL} ${LIBXEXT} ${LIBX11} ${LIBPTHREAD}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
