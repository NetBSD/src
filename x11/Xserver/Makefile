#	$NetBSD: Makefile,v 1.7 2003/12/23 14:44:07 rtr Exp $

.include <bsd.own.mk>

.if ${MACHINE} != "sparc" && ${MACHINE} != "sparc64"
SUBDIR.md=fontbase Xfont
.endif

SUBDIR=	include \
	.WAIT \
	dix os mi printer Xext xkb xinput GL lbx dbe record randr render \
	xtrap mfb cfb cfb16 cfb24 cfb32 fb hw ${SUBDIR.md} \
	.WAIT \
	Xserver

XTODO=	afb miext/shadow miext/layer hw/xfree86

.PATH:	${X11SRCDIR.xc}/programs/Xserver
MAN=	Xserver.1

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
