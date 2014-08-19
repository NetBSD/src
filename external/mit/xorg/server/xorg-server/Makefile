#	$NetBSD: Makefile,v 1.19.12.2 2014/08/20 00:00:14 tls Exp $

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
# Use -fvisibility=hidden for symbol_visibility?
PKGCONFIG_SED_FLAGS= \
	-e "s,@sysconfigdir@,${X11ETCDIR},; \
	    s,@symbol_visibility@,,"

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
