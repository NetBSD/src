#	$NetBSD: Makefile,v 1.2.8.1 2008/11/07 23:23:35 snj Exp $

.include <bsd.own.mk>

LIB=	XprintAppUtil

.PATH:	${X11SRCDIR.${LIB}}/src
SRCS=	xpapputil.c

.PATH:	${X11SRCDIR.${LIB}}/include/X11/XprintAppUtil
INCS=	xpapputil.h
INCSDIR=${X11INCDIR}/X11/XprintAppUtil

LIBDPLIBS=	XprintUtil	${.CURDIR}/../libXprintUtil \
		Xp		${.CURDIR}/../libXp \
		X11		${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
