#	$NetBSD: Makefile,v 1.2 2000/05/03 10:50:59 itojun Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/mailstats

PROG=		mailstats
MAN=		mailstats.8

CPPFLAGS+=	-DNOT_SENDMAIL

.include <bsd.prog.mk>
