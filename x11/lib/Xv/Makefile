#	$NetBSD: Makefile,v 1.6 2005/01/21 05:15:36 rtr Exp $

.include <bsd.own.mk>

LIB=	Xv
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	Xv.c

CPPFLAGS+=	${X11FLAGS.THREADS}

LIBDPLIBS=\
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
