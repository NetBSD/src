# $NetBSD: Makefile,v 1.2 2000/02/25 06:22:05 itojun Exp $

PROG=	route6d
MAN=	route6d.8

CPPFLAGS+=-Dss_len=__ss_len -Dss_family=__ss_family -DADVAPI -DINET6 \
	-DHAVE_GETIFADDRS

.include <bsd.prog.mk>
