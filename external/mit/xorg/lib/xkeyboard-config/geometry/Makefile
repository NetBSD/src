#	$NetBSD: Makefile,v 1.2.2.1 2011/03/05 15:09:11 bouyer Exp $

.include <bsd.own.mk>

XKBNAME=	geometry
FILES=		README amiga ataritt chicony dell everex fujitsu hhk hp \
		keytronic kinesis macintosh microsoft nec nokia northgate \
		pc sanwa sony sun thinkpad typematrix winbook

SUBDIR=		digital_vndr sgi_vndr

XKBDIR=		1

.include "../Makefile.xkbdata"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
