#	$NetBSD: Makefile,v 1.4 2005/01/21 05:15:36 rtr Exp $

.include <bsd.own.mk>

LIB=	XvMC
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XvMC.c

CPPFLAGS+=	${X11FLAGS.THREADS}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
