#	$NetBSD: Makefile,v 1.1.1.1 2001/01/06 13:26:27 martin Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	holidays.D.SH isdnd.rates.A isdnd.rates.D isdnd.rates.F \
	isdnd.rates.L isdnd.rates.UK.BT isdnd.rc.sample
FILESDIR=/usr/share/examples/isdn
.endif
MKOBJ=	no

.include <bsd.prog.mk>
