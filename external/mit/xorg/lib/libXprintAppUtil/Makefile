#	$NetBSD: Makefile,v 1.2.2.3 2008/09/24 16:35:04 wrstuden Exp $

.include <bsd.own.mk>

LIB=	XprintAppUtil

.PATH:	${X11SRCDIR.${LIB}}/src
SRCS=	xpapputil.c

.PATH:	${X11SRCDIR.${LIB}}/include/X11/XprintAppUtil
INCS=	xpapputil.h
INCSDIR=${X11INCDIR}/X11/XprintAppUtil

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
