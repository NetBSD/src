#	$NetBSD: Makefile,v 1.2 2003/09/13 20:19:09 lukem Exp $

NOMAN=	1

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/lib/X11/util

HOSTPROG=	makekeys
HOST_CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}

.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
