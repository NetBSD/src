# $NetBSD: Makefile,v 1.1 1999/07/02 11:47:12 itojun Exp $

PROG=	route6d
MAN=	route6d.8

CPPFLAGS+=	-Dss_len=__ss_len -Dss_family=__ss_family -DADVAPI -DINET6

.include <bsd.prog.mk>
