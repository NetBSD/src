#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:28 haad Exp $

NOMAN=	1

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.X11}/src/util

HOSTPROG=	makekeys
HOST_CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}

.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
