#	$NetBSD: Makefile,v 1.3 2004/01/05 02:29:30 rtr Exp $

.include <bsd.own.mk>

LIB=	XvMC
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XvMC.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
