#	$NetBSD: Makefile,v 1.8 2003/01/09 16:57:46 mrg Exp $

# XXX Temporary for NO_SENDMAIL
.include <bsd.own.mk>

SUBDIR=		dbsym map-mbone mdsetimage mrinfo mrouted mtrace postfix

.if !defined(NO_SENDMAIL) || ${NO_SENDMAIL} == "no"
SUBDIR+=	sendmail
.endif

.include <bsd.subdir.mk>
