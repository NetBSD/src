#	$NetBSD: Makefile,v 1.2 2001/12/12 01:49:16 tv Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	holidays.D.SH isdnd.rates.A isdnd.rates.D isdnd.rates.F \
	isdnd.rates.L isdnd.rates.UK.BT isdnd.rc.sample
FILESDIR=/usr/share/examples/isdn
.endif
NOOBJ=	# defined

.include <bsd.prog.mk>
