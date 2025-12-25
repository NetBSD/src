#	$NetBSD: Makefile,v 1.6 2025/12/25 01:15:00 thorpej Exp $

.include <bsd.own.mk>

SUBDIR=	dhcp mozilla-certdata

.if (${MKBIND} != "no")
SUBDIR+= bind
.endif

.include <bsd.subdir.mk>
