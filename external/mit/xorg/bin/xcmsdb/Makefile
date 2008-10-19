#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:00 haad Exp $

.include <bsd.own.mk>

PROG=	xcmsdb
SRCS=	xcmsdb.c loadData.c

LDADD+=	-lX11
DPADD+=	${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
