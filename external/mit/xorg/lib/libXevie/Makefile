#	$NetBSD: Makefile,v 1.3 2008/10/05 02:05:39 cube Exp $

.include <bsd.own.mk>

LIB=	Xevie

XEVIEDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XEVIEDIR}/src
.PATH:		${XEVIEDIR}/man
.PATH:		${XEVIEDIR}/include/X11/extensions
SRCS=		Xevie.c

INCS=	Xevie.h
INCSDIR=${X11INCDIR}/X11/extensions

MAN=	Xevie.3
MLINKS=	Xevie.3 XevieQueryVersion.3 \
	Xevie.3 XevieStart.3 \
	Xevie.3 XevieSelectInput.3 \
	Xevie.3 XevieSendEvent.3 \
	Xevie.3 XevieEnd.3

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
