#	$NetBSD: Makefile,v 1.19.12.1 2013/06/23 06:28:48 tls Exp $

.include <bsd.own.mk>

.include "Makefile.common"

.if ${XSERVER_NEEDS_XFONT} != "no"
SUBDIR.xfont=Xfont fontbase
.else
SUBDIR.xfont=
.endif

SUBDIR=		doc include .WAIT
SUBDIR+=	glx
SUBDIR+=	dix fb mi Xext miext os randr render 
SUBDIR+=	Xi xkb xkbstubs dbe record xfixes
SUBDIR+=	damageext composite config .WAIT
SUBDIR+=	hw

.PATH:	${X11SRCDIR.xorg-server}

PKGCONFIG=	xorg-server

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
