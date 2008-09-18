#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:54 wrstuden Exp $

NOMAN=	1

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.X11}/src/util

HOSTPROG=	makekeys
HOST_CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}

.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
