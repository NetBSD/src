#	$NetBSD: Makefile,v 1.4 2003/12/20 01:39:31 rtr Exp $

.include <bsd.own.mk>

SUBDIR=	include \
	.WAIT \
	dix os mi printer Xext xkb xinput GL lbx dbe record randr render \
	xtrap mfb cfb cfb16 cfb24 cfb32 fb hw Xfont lbxutil \
	.WAIT \
	Xserver

XTODO=	afb miext/shadow miext/layer hw/xfree86

.PATH:	${X11SRCDIR.xc}/programs/Xserver
MAN=	Xserver.1

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
