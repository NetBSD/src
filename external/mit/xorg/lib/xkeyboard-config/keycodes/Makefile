#	$NetBSD: Makefile,v 1.1.2.2 2009/09/17 04:24:33 snj Exp $

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
