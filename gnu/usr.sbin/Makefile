#	$NetBSD: Makefile,v 1.7 2002/10/01 03:50:11 itojun Exp $

# XXX Temporary for NO_SENDMAIL
.include <bsd.own.mk>

SUBDIR=		dbsym map-mbone mdsetimage mrinfo mrouted mtrace postfix

.if !defined(NO_SENDMAIL)
SUBDIR+=	sendmail
.endif

.include <bsd.subdir.mk>
