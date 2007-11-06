#	$NetBSD: Makefile,v 1.1.2.2 2007/11/06 23:11:08 matt Exp $

LIB=bind9

.include <bsd.own.mk>

.include "${NETBSDSRCDIR}/usr.sbin/bind/Makefile.inc"

DIST=	${IDIST}/lib/bind9
.include "${DIST}/api"

.PATH.c:	${DIST}

SRCS=	check.c getaddresses.c version.c

.include <bsd.lib.mk>
