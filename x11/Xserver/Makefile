#	$NetBSD: Makefile,v 1.2 2003/09/13 20:17:58 lukem Exp $

.include <bsd.own.mk>

SUBDIR=	include \
	.WAIT \
	GL Xext cfb cfb16 cfb24 cfb32 dbe dix fb lbx mfb mi os printer \
	randr record render xinput xkb xtrap hw \
	.WAIT \
	Xserver

XTODO=	afb miext/shadow miext/layer hw/xfree86

.PATH:	${X11SRCDIR.xc}/programs/Xserver
MAN=	Xserver.1

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
