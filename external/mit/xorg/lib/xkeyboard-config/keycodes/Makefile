#	$NetBSD: Makefile,v 1.2.12.1 2013/06/23 06:28:46 tls Exp $

.include <bsd.own.mk>

XKBNAME=	keycodes
FILES=		README aliases amiga ataritt empty evdev fujitsu hp ibm \
		macintosh olpc sony sun xfree86 xfree98

SUBDIR=		digital_vndr sgi_vndr

XKBDIR=		1

.include "../Makefile.xkbdata"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
