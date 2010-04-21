#	$NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:24:37 matt Exp $

.include <bsd.own.mk>

XKBNAME=	geometry
FILES=		README amiga ataritt chicony dell everex fujitsu hhk hp \
		keytronic kinesis macintosh microsoft nec northgate pc \
		sony sun thinkpad winbook

SUBDIR=		digital_vndr sgi_vndr

XKBDIR=		1

.include "../Makefile.xkbdata"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
