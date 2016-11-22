#	$NetBSD: Makefile,v 1.4 2016/11/22 19:51:37 snj Exp $

.include <bsd.own.mk>

XKBNAME=	keycodes
FILES=		README aliases amiga ataritt empty evdev fujitsu hp ibm \
		jolla macintosh olpc sony sun xfree86 xfree98

SUBDIR=		digital_vndr sgi_vndr

XKBDIR=		1

.include "../Makefile.xkbdata"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
