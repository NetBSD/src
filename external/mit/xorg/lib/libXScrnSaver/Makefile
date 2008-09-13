#	$NetBSD: Makefile,v 1.2 2008/09/13 04:32:30 cube Exp $

.include <bsd.own.mk>

LIB=	Xss
.PATH:	${X11SRCDIR.XScrnSaver}/src

SRCS=	XScrnSaver.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}
#CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}

LIBDPLIBS=	X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	XScrnSaver

.include <bsd.x11.mk>
.include <bsd.lib.mk>
