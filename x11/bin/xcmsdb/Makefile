#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:14 lukem Exp $

.include <bsd.own.mk>

PROG=	xcmsdb
SRCS=	xcmsdb.c loadData.c

LDADD+=	-lX11
DPADD+=	${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
