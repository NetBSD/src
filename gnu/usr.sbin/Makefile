#	$NetBSD: Makefile,v 1.6 2002/10/01 03:30:51 itojun Exp $

# XXX Temporary for NO_SENDMAIL
.include <bsd.own.mk>

SUBDIR=		dbsym mdsetimage mrouted postfix

.if !defined(NO_SENDMAIL)
SUBDIR+=	sendmail
.endif

.include <bsd.subdir.mk>
