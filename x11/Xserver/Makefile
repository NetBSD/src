#	$NetBSD: Makefile,v 1.3 2003/09/23 23:21:02 lukem Exp $

.include <bsd.own.mk>

SUBDIR=	include \
	.WAIT \
	dix os mi printer Xext xkb xinput GL lbx dbe record randr render \
	xtrap mfb cfb cfb16 cfb24 cfb32 fb hw \
	.WAIT \
	Xserver

XTODO=	afb miext/shadow miext/layer hw/xfree86

.PATH:	${X11SRCDIR.xc}/programs/Xserver
MAN=	Xserver.1

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
