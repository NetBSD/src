#	$NetBSD: Makefile,v 1.3 2008/11/06 22:28:26 veego Exp $

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
