#	$NetBSD: Makefile,v 1.3 2001/12/12 12:24:22 lukem Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	holidays.D.SH isdnd.rates.A isdnd.rates.D isdnd.rates.F \
	isdnd.rates.L isdnd.rates.UK.BT isdnd.rc.sample
FILESDIR=/usr/share/examples/isdn
.endif

.include <bsd.prog.mk>
