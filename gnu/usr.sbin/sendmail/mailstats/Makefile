#	$NetBSD: Makefile,v 1.3 2002/09/19 03:09:55 lukem Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/mailstats

PROG=		mailstats
MAN=		mailstats.8

CPPFLAGS+=	-DNOT_SENDMAIL

.include <bsd.prog.mk>
