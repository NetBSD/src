# $NetBSD: Makefile,v 1.4 2001/02/07 13:46:04 itojun Exp $

PROG=	route6d
MAN=	route6d.8

CPPFLAGS+=-Dss_len=__ss_len -Dss_family=__ss_family -DINET6

.include <bsd.prog.mk>
