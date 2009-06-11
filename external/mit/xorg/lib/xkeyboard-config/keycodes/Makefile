#	$NetBSD: Makefile,v 1.1 2009/06/11 01:49:21 mrg Exp $

.include <bsd.own.mk>

XKBNAME=	keycodes
FILES=		README aliases amiga ataritt evdev fujitsu hp ibm \
		macintosh sony sun xfree86 xfree98

SUBDIR=		digital_vndr sgi_vndr

XKBDIR=		1

.include "../Makefile.xkbdata"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
