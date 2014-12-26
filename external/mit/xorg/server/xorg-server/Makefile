#	$NetBSD: Makefile,v 1.19.8.2 2014/12/26 07:29:34 msaitoh Exp $

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
	    s,@SDK_REQUIRED_MODULES@,xproto >= 7.0.17 randrproto >= 1.2.99.3 renderproto >= 0.11 xextproto >= 7.1.99 inputproto >= 1.9.99.902 kbproto >= 1.0.3 fontsproto,; \
	    s,@symbol_visibility@,,"

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
