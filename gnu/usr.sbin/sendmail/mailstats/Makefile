#	$NetBSD: Makefile,v 1.1.1.1 2000/02/08 13:45:35 tron Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/mailstats

PROG=		mailstats
MAN=		mailstats.8

CPPFLAGS+=	-I${DIST}/sendmail/src -DNOT_SENDMAIL

.include <bsd.prog.mk>
