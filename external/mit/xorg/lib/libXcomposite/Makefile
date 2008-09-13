#	$NetBSD: Makefile,v 1.2 2008/09/13 04:32:30 cube Exp $

.include <bsd.own.mk>

LIB=	Xcomposite

XCOMPOSITEDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XCOMPOSITEDIR}/src
.PATH:		${XCOMPOSITEDIR}/include/X11/extensions
SRCS=		Xcomposite.c

INCS=	Xcomposite.h
INCSDIR=${X11INCDIR}/X11/extensions

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
