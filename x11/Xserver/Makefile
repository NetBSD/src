#	$NetBSD: Makefile,v 1.11 2004/01/18 05:24:45 rtr Exp $

.include <bsd.own.mk>

.include "Makefile.common"

.if ${XSERVER_NEEDS_XFONT} != "no"
SUBDIR.xfont=fontbase Xfont
.endif

.if ${XSERVER_HAS_CFB} == "no"
SUBDIR.cfb=
.else
SUBDIR.cfb=cfb cfb16 cfb24 cfb32
.endif

.if ${XSERVER_HAS_FB} == "no"
SUBDIR.fb=
.else
SUBDIR.fb=fb
.endif

.if ${XSERVER_HAS_AFB} != "no"
SUBDIR.afb=afb
.else
SUBDIR.afb=
.endif

SUBDIR=		include
SUBDIR+=	.WAIT dix os mi printer Xext xkb xinput GL lbx dbe record
SUBDIR+=	mfb randr render xtrap
SUBDIR+=	${SUBDIR.cfb} ${SUBDIR.fb} hw ${SUBDIR.xfont} ${SUBDIR.afb}
SUBDIR+=	.WAIT Xserver

XTODO=	afb miext/shadow miext/layer

.PATH:	${X11SRCDIR.xc}/programs/Xserver
MAN=	Xserver.1

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
