#	$NetBSD: Makefile,v 1.3.10.2 2017/03/20 06:56:49 pgoyette Exp $

.include <bsd.own.mk>

XKBNAME=	keycodes
FILES=		README aliases amiga ataritt empty evdev fujitsu hp ibm \
		jolla macintosh olpc sony sun xfree86 xfree98

SUBDIR=		digital_vndr sgi_vndr

.include "../Makefile.xkbdata"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
