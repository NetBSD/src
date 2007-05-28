# $NetBSD: Makefile,v 1.4 2007/05/28 12:06:39 tls Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network client

PROG=	rip6query
MAN=	rip6query.8

CPPFLAGS+=	-DINET6 -I${NETBSDSRCDIR}/usr.sbin/route6d

.include <bsd.prog.mk>
