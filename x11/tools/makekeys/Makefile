#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:38:55 lukem Exp $

NOMAN=	1

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/lib/X11/util

HOSTPROG=	makekeys
HOST_CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}

.include <bsd.hostprog.mk>
.include <bsd.x11.mk>
