#	$NetBSD: Makefile,v 1.3.26.1 2017/01/07 08:56:00 pgoyette Exp $

.include <bsd.own.mk>

XKBNAME=	geometry
FILES=		README amiga ataritt chicony dell everex fujitsu hhk hp \
		keytronic kinesis macintosh microsoft nec nokia northgate \
		pc sanwa sony sun teck thinkpad typematrix winbook

SUBDIR=		digital_vndr sgi_vndr

XKBDIR=		1

.include "../Makefile.xkbdata"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
