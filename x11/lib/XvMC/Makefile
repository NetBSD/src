#	$NetBSD: Makefile,v 1.5 2005/09/24 13:51:47 chs Exp $

.include <bsd.own.mk>

LIB=	XvMC
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XvMC.c

LIBDPLIBS=\
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

CPPFLAGS+=	${X11FLAGS.THREADS}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
