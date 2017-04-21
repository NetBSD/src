#	$NetBSD: Makefile,v 1.4.2.1 2017/04/21 16:53:00 bouyer Exp $

.include <bsd.own.mk>

XKBNAME=	keycodes
FILES=		README aliases amiga ataritt empty evdev fujitsu hp ibm \
		jolla macintosh olpc sony sun xfree86 xfree98

SUBDIR=		digital_vndr sgi_vndr

.include "../Makefile.xkbdata"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
