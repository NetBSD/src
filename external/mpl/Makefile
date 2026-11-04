#	$NetBSD: Makefile,v 1.5.2.1 2026/11/04 23:15:29 perseant Exp $

.include <bsd.own.mk>

SUBDIR=	dhcp mozilla-certdata

.if (${MKBIND} != "no")
SUBDIR+= bind
.endif

.include <bsd.subdir.mk>
