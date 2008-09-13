#	$NetBSD: Makefile,v 1.2 2008/09/13 04:32:31 cube Exp $

.include <bsd.own.mk>

LIB=	XvMC
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/extensions

SRCS=	XvMC.c

INCS=	XvMClib.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=	${X11FLAGS.THREADLIB}

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
