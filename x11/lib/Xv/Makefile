#	$NetBSD: Makefile,v 1.5 2004/08/05 01:28:48 lukem Exp $

.include <bsd.own.mk>

LIB=	Xv
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	Xv.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LIBDPLIBS=\
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
