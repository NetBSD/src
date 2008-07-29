#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

LIB=	Xss
.PATH:	${X11SRCDIR.XScrnSaver}/src

SRCS=	XScrnSaver.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}
#CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}

LIBDPLIBS=	X11	${.CURDIR}/../libX11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
