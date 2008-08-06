#	$NetBSD: Makefile,v 1.12 2008/08/06 04:15:06 mrg Exp $

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

.if ${XSERVER_HAS_CFB} != "no"
SUBDIR.cfb=cfb cfb32
.else
SUBDIR.cfb=
.endif

.if ${XSERVER_HAS_FB} != "no"
SUBDIR.fb=fb
.else
SUBDIR.fb=
.endif

.if ${XSERVER_HAS_AFB} != "no"
SUBDIR.afb=afb
.else
SUBDIR.afb=
.endif

.if ${XSERVER_HAS_IPLAN} != "no"
SUBDIR.iplan=iplan2p4 iplan2p8
.else
SUBDIR.iplan=
.endif

SUBDIR=		doc include .WAIT
SUBDIR+=	dix ${SUBDIR.fb} mi Xext ${SUBDIR.miext} os randr render 
SUBDIR+=	Xi xkb dbe mfb ${SUBDIR.afb} ${SUBDIR.cfb} record xfixes
SUBDIR+=	damageext XTrap composite exa config hw

# XXX stuff installed by modular-xorg-server we need to deal with:
#libpcidata
#libshadow
#libshadowfb
#libvbe
#libvgahw
#libxaa

#bin/X
#bin/Xorg
#bin/xorgcfg
#bin/xorgconfig

.PATH:	${X11SRCDIR.xorg-server}

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
