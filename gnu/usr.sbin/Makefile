#	$NetBSD: Makefile,v 1.5.2.2 2002/12/08 10:20:46 he Exp $

# XXX Temporary for NO_SENDMAIL
.include <bsd.own.mk>

SUBDIR=		dbsym map-mbone mdsetimage mrinfo mrouted mtrace postfix

.if !defined(NO_SENDMAIL)
SUBDIR+=	sendmail
.endif

.include <bsd.subdir.mk>
