#	$NetBSD: Makefile,v 1.1.1.1.4.1 2014/05/22 14:09:37 yamt Exp $

.include <bsd.own.mk>

PROG=	xcmsdb
SRCS=	xcmsdb.c loadData.c

LDADD+=	-lX11
DPADD+=	${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
