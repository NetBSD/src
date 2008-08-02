#	$NetBSD: Makefile,v 1.2 2008/08/02 04:46:57 mrg Exp $

.include <bsd.own.mk>

.include "Makefile.common"

.if ${XSERVER_NEEDS_XFONT} != "no"
.if ${MKX11LOADABLE} != "no"
SUBDIR.xfont=Xfont
.endif
SUBDIR.xfont+=fontbase
.else
SUBDIR.xfont=
.endif

.if ${XSERVER_NEEDS_MIEXT} != "no"
SUBDIR.miext=miext
.else
SUBDIR.miext=
.endif

.if ${XSERVER_HAS_CFB} != "no"
SUBDIR.cfb=cfb cfb16 cfb24 cfb32
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

.if 0
### this is the desired version ###
SUBDIR=		doc include .WAIT
SUBDIR+=	dix ${SUBDIR.fb} mi Xext ${SUBDIR.miext} os randr render 
SUBDIR+=	Xi xkb dbe mfb ${SUBDIR.afb} {SUBDIR.cfb} record xfixes
SUBDIR+=	damageext XTrap composite exa config hw
.else
### this is the current working set
SUBDIR=		doc include .WAIT
SUBDIR+=	dix ${SUBDIR.fb} mi ${SUBDIR.miext} randr render 
.endif

.if 0
### this is the old version
SUBDIR=		include
SUBDIR+=	.WAIT dix os mi printer Xext xkb xinput GL lbx dbe record
SUBDIR+=	mfb randr render xtrap composite damage xfixes
SUBDIR+=	${SUBDIR.cfb} ${SUBDIR.fb} ${SUBDIR.iplan} hw
SUBDIR+=	${SUBDIR.xfont} ${SUBDIR.afb} ${SUBDIR.miext}
SUBDIR+=	.WAIT Xserver
.endif

.PATH:	${X11SRCDIR.xorg-server}
MAN=	Xserver.1

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
