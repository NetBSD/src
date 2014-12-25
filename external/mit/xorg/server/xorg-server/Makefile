#	$NetBSD: Makefile,v 1.19.8.1 2014/12/25 05:05:02 msaitoh Exp $

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
# Use -fvisibility=hidden for symbol_visibility?
PKGCONFIG_SED_FLAGS= \
	-e "s,@sysconfigdir@,${X11ETCDIR},; \
	    s,@symbol_visibility@,,"

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
