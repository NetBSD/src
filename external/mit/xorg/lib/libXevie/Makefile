#	$NetBSD: Makefile,v 1.3.4.2 2008/10/19 22:41:09 haad Exp $

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
