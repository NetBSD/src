#	$NetBSD: Makefile,v 1.17.6.2 2009/09/17 04:24:37 snj Exp $

.include <bsd.own.mk>

.include "Makefile.common"

.if ${XSERVER_NEEDS_XFONT} != "no"
SUBDIR.xfont=Xfont fontbase
.else
SUBDIR.xfont=
.endif

.if ${XSERVER_NEEDS_MIEXT} != "no"
SUBDIR.miext=miext
.else
SUBDIR.miext=
.endif

.if ${XSERVER_HAS_IPLAN} != "no"
SUBDIR.iplan=iplan2p4 iplan2p8
.else
SUBDIR.iplan=
.endif

SUBDIR=		doc include .WAIT
SUBDIR+=	glx
SUBDIR+=	dix fb mi Xext ${SUBDIR.miext} os randr render 
SUBDIR+=	Xi xkb xkbstubs dbe record xfixes
SUBDIR+=	damageext composite config .WAIT
SUBDIR+=	hw

.PATH:	${X11SRCDIR.xorg-server}

PKGCONFIG=	xorg-server

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
