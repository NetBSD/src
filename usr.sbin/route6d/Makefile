# $NetBSD: Makefile,v 1.2.4.1 2000/07/15 08:52:28 itojun Exp $

PROG=	route6d
MAN=	route6d.8

CPPFLAGS+=-Dss_len=__ss_len -Dss_family=__ss_family -DINET6 -DHAVE_GETIFADDRS

.include <bsd.prog.mk>
